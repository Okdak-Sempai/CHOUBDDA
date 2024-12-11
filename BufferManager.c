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

static buffer *rotate()
{
    buffer *cur_tail = bufferManager->bufferTail;

    cur_tail->prev->next = NULL;
    bufferManager->bufferTail = cur_tail->prev;
    cur_tail->prev = NULL;

    bufferManager->bufferHead->prev = cur_tail;
    cur_tail->next = bufferManager->bufferHead;
    bufferManager->bufferHead = cur_tail;

    return cur_tail;
}

static buffer* LRU()
{
    return rotate();
}

static buffer* MRU()
{
    if (bufferManager->bufferTail->bufferPageId == NULL)
        return rotate();
    return bufferManager->bufferHead;
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

    if (buf->prev == NULL) // buf is head
        return buf;

    buffer *next = buf->next;
    buffer *prev = buf->prev;

    if (next != NULL) // buf isn't the tail
        next->prev = prev;
    prev->next = next;

    buf->prev = NULL;
    buf->next = bufferManager->bufferHead;

    if (bufferManager->bufferHead == prev)
        bufferManager->bufferHead->prev = buf;
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

uint8_t *GetPage(PageId *pageId) {
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

        ReadPage(pageId, buf->content);
        buf->bufferPageId = pageId;
        buf->pin_count = 0;
        buf->flagdirty = 0;
    }
    assert(buf == bufferManager->bufferHead);
    assert(buf->content != NULL);
    buf->pin_count++;


    return buf->content;
}

void FreePage(PageId *pageId, int valdirty) {
    buffer *buf = find_in_buf(pageId);
    if (buf == NULL)
        return;

    buf->pin_count--;
    buf->flagdirty = valdirty;

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
    }
}