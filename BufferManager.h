#ifndef SHINBDDA_BUFFERMANAGER_H
#define SHINBDDA_BUFFERMANAGER_H

#include "Structures.h"
#include "PageId.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer buffer;

struct buffer
{
    PageId* bufferPageId;
    int pin_count;
    int flagdirty;
    uint8_t *content;

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
uint8_t *GetPage(PageId *pageId);
void FreePage(PageId *pageId, int valdirty);
void SetCurrentReplacementPolicy (Policy);
void FlushBuffers();

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_BUFFERMANAGER_H
