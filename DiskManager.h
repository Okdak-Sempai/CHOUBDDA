#ifndef SHINBDDA_DISKMANAGER_H
#define SHINBDDA_DISKMANAGER_H

#include "Structures.h"
#include "DBConfig.h"
#include "PageId.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DiskManager
/*
* DiskManager Data
*/
{
    DBConfig* config; //config structure
    PageId** allocated;//List of Allocated PageId
    PageId** desalocated;//List of Desalocated PageId
    int size;//Size of both list.
}DiskManager;

extern DiskManager* diskManager;


static int count_page_id(PageId **ids, int size);
int diskInit();
int diskFREE();
PageId* AllocPage();
void DeallocPage(PageId* pageid);
void ReadPage(PageId* pageid, unsigned char* buff);
void WritePage(PageId* pageid, unsigned char* buff );
void SaveState();
void LoadState();

PageId *FindPageId(PageId pageId);

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_DISKMANAGER_H
