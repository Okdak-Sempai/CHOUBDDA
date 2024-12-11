
#ifndef SHINBDDA_PAGEID_H
#define SHINBDDA_PAGEID_H

#include "Structures.h"
#include "DBConfig.h"
#include "DiskManager.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PageId
/*
*   PageId data
*/
{
    int FileIdx; //x of the Fx.rsbd so  index// FICHIER
    int PageIdx; //Indice de la page fichier
}PageId;

typedef struct PageIdList
/*
* The exhaustive list of all the PageId.
*/
{
    int size; // Size of the list array, so the number of PageId.
    int numnerofFiles; // Number of Files.
    PageId** list; //List of PageId
}PageIdList;

PageIdList* InitiatiateTables(char* folderpath,int filesize,int pagesize, int number);
PageId* addTable(char* folderpath,int filesize,int pagesize,int toadd,PageIdList* pageIdlisted, DiskManager* diskManager);
char* getPageIdFile(PageId* pageid);

#ifdef __cplusplus
}
#endif


#endif //SHINBDDA_PAGEID_H


