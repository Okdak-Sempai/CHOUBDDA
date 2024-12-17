#ifndef SHINBDDA_BUFFERMANAGER_H
#define SHINBDDA_BUFFERMANAGER_H

#include "Structures.h"
#include "PageId.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer buffer;

typedef struct OwnerSrc
{
    const char *function;
    const char *filename;
    size_t line;
} OwnerSrc;

struct buffer
{
    PageId* bufferPageId;
    int pin_count;
    int flagdirty;
    uint8_t *content;

    size_t nb_owners;
    OwnerSrc *owners;

    buffer* next;
    buffer* prev;
};



typedef struct BufferManager
/*
*   bufferManager data
*/
{
    buffer* bufferHead; // Call Config->dm_buffercount for init
    buffer* bufferTail;
} BufferManager;

extern BufferManager *bufferManager;

////
void constructBufferManager();
void clearBufferManager();
uint8_t *__GetPage(PageId *pageId, const char *function, const char *filename, size_t line);
void __FreePage(PageId *pageId, int valdirty, const char *function, const char *filename, size_t line);
void SetCurrentReplacementPolicy (Policy);
void FlushBuffers();

#define GetPage(pageId) __GetPage(pageId, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#define FreePage(pageId, valdirty) __FreePage(pageId, valdirty, __PRETTY_FUNCTION__, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_BUFFERMANAGER_H
