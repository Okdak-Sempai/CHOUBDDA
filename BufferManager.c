#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "BufferManager.h"


BufferManager *bufferManager;

void constructBufferManager()

{
    bufferManager = (BufferManager*) malloc(sizeof(BufferManager));
    if (bufferManager == NULL)
    {
        perror("error: malloc config:");
        abort();
    }

    bufferManager->bufferHead = calloc(1, sizeof *bufferManager->bufferHead);

    buffer *buf = bufferManager->bufferHead;
    int i = 0;
    for (; i < config->dm_buffercount - 1; i++)
    {
        buf->next = calloc(1, sizeof *bufferManager->bufferHead);
        buf->next->prev = buf;
        buf = buf->next;
    }

    bufferManager->bufferTail = buf;
}

static buffer* LRU()
{
    buffer *buf = bufferManager->bufferTail;
    for (; buf; buf = buf->prev)
        if (!buf->pin_count || buf->flagdirty)
            break;
    if (!buf || buf == bufferManager->bufferHead)
        return buf;

    buffer *next = buf->next;
    buffer *prev = buf->prev;

    if (next)
        next->prev = prev;
    else
        bufferManager->bufferTail = prev;
    if (prev)
        prev->next = next;

    bufferManager->bufferHead->prev = buf;
    buf->next = bufferManager->bufferHead;
    buf->prev = NULL;
    bufferManager->bufferHead = buf;

    return buf;
}

static buffer* MRU()
{
    buffer *buf = bufferManager->bufferHead;
    for (; buf; buf = buf->next)
        if (!buf->pin_count && !buf->flagdirty)
            break;
    if (!buf || buf == bufferManager->bufferHead)
        return buf;

    buffer *next = buf->next;
    buffer *prev = buf->prev;

    if (next)
        next->prev = prev;
    else
        bufferManager->bufferTail = prev;
    if (prev)
        prev->next = next;

    bufferManager->bufferHead->prev = buf;
    buf->next = bufferManager->bufferHead;
    buf->prev = NULL;
    bufferManager->bufferHead = buf;

    return buf;
}

static buffer *find_in_buf(PageId *pageId) {
    buffer *buf = bufferManager->bufferHead;

    for (; buf; buf = buf->next)
    {
        if (buf->bufferPageId == pageId)
            break;
    }

    if (buf == NULL)
        return NULL;

    if (buf == bufferManager->bufferHead) // buf is head
        return buf;

    buffer *next = buf->next;
    buffer *prev = buf->prev;

    if (next)
        next->prev = prev;
    else // buf is tail
        bufferManager->bufferTail = prev;
    if (prev)
        prev->next = next;

    bufferManager->bufferHead->prev = buf;
    buf->next = bufferManager->bufferHead;
    buf->prev = NULL;
    bufferManager->bufferHead = buf;

    return buf;
}

void clearBufferManager() {
    buffer * buf = bufferManager->bufferHead;
    buffer * next = buf->next;

    while (next) {
        next = buf->next;

        free(buf->content);
        free(buf);

        buf = next;
    }

    free(bufferManager);
}

uint8_t *__GetPage(PageId *pageId, const char *function, const char *filename, size_t line) {
    buffer *buf = find_in_buf(pageId);
    /*
     *  buffer *(*fn[2])();
     *  fn[POLICY_LRU] = LRU;
     *  fn[POLICY_MRU] = MRU;
     *  buf = fn[config->dm_policy]();
     */
    if (!buf)
    {
        buf = (config->dm_policy == POLICY_LRU ? LRU : MRU)();
        if (buf->flagdirty)
        {
            // manage dirty, pin count (check if correct)
            WritePage(buf->bufferPageId, buf->content);
            memset(buf->content, 0, config->pagesize);
        }
        else if (!buf->content)
        {
            buf->content = malloc(config->pagesize * sizeof(uint8_t));
        }

        assert(buf->nb_owners == 0);

        ReadPage(pageId, buf->content);
        buf->bufferPageId = pageId;
        buf->pin_count = 0;
        buf->flagdirty = 0;
    }
    assert(buf == bufferManager->bufferHead);
    assert(buf->content != NULL);
    buf->pin_count++;

    buf->nb_owners++;
    OwnerSrc *tmp = realloc(buf->owners, buf->nb_owners * sizeof(OwnerSrc));
    assert(tmp != NULL);
    buf->owners = tmp;
    buf->owners[buf->nb_owners - 1].filename = filename;
    buf->owners[buf->nb_owners - 1].line = line;
    buf->owners[buf->nb_owners - 1].function = function;

    return buf->content;
}

void __FreePage(PageId *pageId, int valdirty, const char *function, const char *filename, size_t line) {
    buffer *buf = find_in_buf(pageId);
    if (buf == NULL)
        return;

    buf->pin_count--;
    buf->flagdirty = buf->flagdirty || valdirty; // never clear flag dirty on free, we should clean it if we actually write the page

    if (buf->nb_owners == 1)
    {
        free(buf->owners);
        buf->owners = NULL;
        buf->nb_owners = 0;
    }
    else
    {
        size_t i = 0;
        for (; i < buf->nb_owners; i++)
            if (strcmp(buf->owners[i].filename, filename) == 0 && strcmp(buf->owners[i].function, function) == 0)
                break;
        assert(i < buf->nb_owners);

        if (i < buf->nb_owners - 1)
            memmove(buf->owners + i, buf->owners + i + 1, (buf->nb_owners - i - 1) * sizeof(OwnerSrc));
        OwnerSrc *tmp = realloc(buf->owners, (buf->nb_owners - 1) * sizeof(OwnerSrc));
        assert(tmp != NULL);
        buf->owners = tmp;
        buf->nb_owners--;
    }

    if (buf->pin_count <= 0)
        buf->pin_count = 0;
}

void SetCurrentReplacementPolicy (Policy paul)
{
    config->dm_policy=paul;
}

void FlushBuffers() {
    for (buffer *buf = bufferManager->bufferHead; buf; buf = buf->next)
    {
        assert(buf->nb_owners == 0 && buf->pin_count == 0);
        if (buf->flagdirty)
        {
            assert(buf->content);
            // manage dirty, pin count (check if correct)
            WritePage(buf->bufferPageId, buf->content);
            memset(buf->content, 0, config->pagesize);
        }

        buf->flagdirty = 0;
        buf->pin_count = 0;
        buf->bufferPageId = NULL;
        free(buf->content);
        buf->content = NULL;
        free(buf->owners);
        buf->owners = NULL;
    }
}