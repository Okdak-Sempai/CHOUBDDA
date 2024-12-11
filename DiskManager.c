#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "DiskManager.h"
#include "Tools_L.h"
#include "DBConfig.h"
#include "PageId.h"
#include <errno.h>

static PageIdList* pageidlist;
DiskManager* diskManager;
int defaultnumberoffiles = 3;

static int count_page_id(PageId **ids, int size)
/*
* Includes:
*   <stddef.h> [NULL]
*
*   "PageId.h" [PageId]
*   "DiskManager.h" [DiskManager]
* Params:
*   PageId **ids = The Array of PageIds.
*   int size = The size of the array of PageIds.
* Return:
*   int => Returns the size of not NULL Elements of the array
* Description:
*   This function returns the number of not NULL elements of an array.
* Malloc:
*   None.
* Notes:
*   None.
*/
{
    int nb = 0;

    for (int i = 0; i < size; i++)
        nb += ids[i] != NULL;

    return nb;
}

int diskInit()
/*
* Includes:
*   <stdio.h> [sizeof(); perror()]
*   <stddef.h> [NULL]
*   <stdlib.h> [malloc(); free()]
*
*   "PageId.h" [PageIdList; PageId; InitiatiateTables()]
*   "DBConfig.h" [DBConfig]
*   "DiskManager.h" [DiskManager, LoadState()]
* Params:
*   DBConfig* config = The DBConfig* config
* Return:
*   int
*      0 = Flawless execution of saved state.
*      1 = Flawless execution of the Init.
*      -3 = Failed diskManager->allocated malloc().
*      -4 = Failed diskManager->desalocated malloc().
* Description:
*   This function Initiates diskManager using DBConfig* configfile.
*   It is stored as a static value, this function shall be called the soonest possible in the main to Initiate diskManager.
*   If it can Load a previous state, it will then create and Initiates the diskManager with LoadState().
* Malloc:
*   diskManager shall be free using diskFREE().
* Notes:
*   None.
*/
{
    diskManager = (DiskManager*) malloc((sizeof(DiskManager)));
    if (diskManager == NULL)
    {
        perror("Memory allocation failed for DiskManager");
        return -2;
    }
    diskManager->config=config;

    //Checks saved state, If no save.dm, launch an init.
    LoadState();
    if (!config->need_init)
    {
        return 0;
    }

    //Init
    pageidlist = InitiatiateTables(config->dbpath,config->dm_maxfilesize,config->pagesize,defaultnumberoffiles);
    diskManager->allocated = (PageId**) malloc(pageidlist->size*sizeof(PageId*));
    if (diskManager->allocated == NULL)
    {
        perror("Memory allocation failed for allocated");
        return -3;
    }
    diskManager->desalocated = (PageId**) malloc(pageidlist->size*sizeof(PageId*));
    if (diskManager->desalocated == NULL)
    {
        perror("Memory allocation failed for desalocated");
        free(diskManager->allocated);
        return -4;
    }
    diskManager->size=pageidlist->size;
    for (int i = 0; i < diskManager->size; i++)
    {
        diskManager->allocated[i]=NULL;
    }
    for (int i = diskManager->size - 1, j=0; i >= 0; i--,j++)//Met la liste a l'envers pour la LIFO
    {

        diskManager->desalocated[j]=pageidlist->list[i];
    }
    //
//    DiskManager* disk =diskManager;
//    printf("Élément %d : FileIdx = %d, PageIdx = %d\n",4,disk->desalocated[4]->FileIdx,disk->desalocated[4]->PageIdx);
//    printf("Élément %d : FileIdx = %d, PageIdx = %d\n",5,disk->desalocated[5]->FileIdx,disk->desalocated[5]->PageIdx);
//    printf("Élément %d : FileIdx = %d, PageIdx = %d\n",6,disk->desalocated[6]->FileIdx,disk->desalocated[6]->PageIdx);
    //
    return 1;
}

int diskFREE()
/*
* Includes:
*   <stddef.h> [NULL]
*   <stdlib.h> [malloc(); free()]
*
*   "PageId.h" [PageIdList; PageId]
*   "DBConfig.h" [DBConfig]
*   "DiskManager.h" [DiskManager]
* Params:
*   None.
* Return:
*   int
*      0 = Flawless execution.
* Description:
*   This function free
*      pageidlist
*      diskManager
* Malloc:
*   Has to be called to free pageidlist and diskManager
* Notes:
*   This free the content of these structs.
*/
{
    for (int i = 0; i < pageidlist->size; i++)
    {
        if (pageidlist->list[i] != NULL)
        {
            free(pageidlist->list[i]);
        }
    }
    free(pageidlist->list);
    free(pageidlist);
    free(diskManager->allocated);
    free(diskManager->desalocated);
    free(diskManager);
    return 0;
}

PageId* AllocPage()
/*
* Includes:
*   <stddef.h> [NULL]
*
*   "PageId.h" [PageIdList; PageId; addTable()]
*   "DBConfig.h" [DBConfig]
*   "DiskManager.h" [DiskManager]
* Params:
*   None.
* Return:
*   PageId* => Returns the Allocated PageId.
* Description:
*   This function Alloc a PageId and returns it.
 *   This stored allocated pages in NeodiskManager->allocated and the others in NeodiskManager->desalocated
 *   When there is no more PageId to yield, creates a new file and PageId.
* Malloc:
*   Nothing to free, carefully. diskFREE() Manages it.
* Notes:
*   Uses LIFO.
*/
{
    //Reasign
    DiskManager* NeodiskManager = diskManager;
    PageId* pageId=NULL;
    //Check if there's page avalaible
    int i = NeodiskManager->size - 1;
    while (i >= 0 && NeodiskManager->desalocated[i] == NULL)
    {
        i--;
    }
    //No page avalaible case
    //Tout est NULL[Donc allocated est FULL & desalocated est VIDE] ajotuer une page et retourner cette page
    if(i==-1)
    {
        //Ajout de la table, on augmente la taille de Diskmanager size
        return addTable(config->dbpath,config->dm_maxfilesize,config->pagesize,1,pageidlist,NeodiskManager);
    }

    //Remove from desalocated, add to alocated and returns it
    int j = 0;
    while (j < NeodiskManager->size && NeodiskManager->allocated[j] != NULL)//Il y a FORCEMENT de la place
    {
        j++;
    }
    PageId* tmp = NeodiskManager->desalocated[i];
    NeodiskManager->desalocated[i]=NeodiskManager->allocated[j];
    NeodiskManager->allocated[j]=tmp;
    return NeodiskManager->allocated[j];
}

void DeallocPage (PageId* pageid)
/*
* Includes:
*   <string.h> [memmove(); memset()]
*
*   "PageId.h" [PageId]
*   "DiskManager.h" [DiskManager; count_page_id()]
* Params:
*   PageId* pageid => The PageId to remove from allocated, put it to desaloacted Array.
* Return:
*   None.
* Description:
*   This function Desalloc the PageId, it makes allocated array move so there is no Holes. And put it in desalocated.
* Malloc:
*   None.
* Notes:
*   Uses LIFO.
*   Should be used with DiskManager struct to ensure that the PageId* is from it.
*/
{
    int pivot = -1;
    int nb_alloc = count_page_id(diskManager->allocated, diskManager->size);
    int nb_dealloc = count_page_id(diskManager->desalocated, diskManager->size);

    for (int i = 0; i < diskManager->size && diskManager->allocated[i]; i++)
    {
        if (diskManager->allocated[i] != pageid)
            continue;

        pivot = i;
        break;
    }

    //Desaloc PageId
    diskManager->desalocated[nb_dealloc] = diskManager->allocated[pivot];

    //Resize allocated and put NULL(so 0)
    memmove(diskManager->allocated + pivot, diskManager->allocated + pivot + 1, (diskManager->size - pivot - 1) * sizeof *diskManager->allocated);
    memset(diskManager->allocated + nb_alloc - 1, 0, (diskManager->size - (nb_alloc - 1)) * sizeof *diskManager->allocated);
}

void ReadPage(PageId* pageid, unsigned char* buff)
//<fcntl.h>
//<sys/mman.h>
{
    char* path = getPageIdFile(pageid);
    int fd = open(path,O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Error setting file size in ReadPage: %s: %s\n", path, strerror(errno));
        free(path);
        return;
    }

    char* pagereaded = mmap(NULL,config->dm_maxfilesize,PROT_READ,MAP_SHARED,fd,0);
    if (pagereaded == MAP_FAILED)
    {
        perror("Error mmap on Read");
        close(fd);
        free(path);
        return;
    }

    memcpy(buff, pagereaded + pageid->PageIdx * config->pagesize, config->pagesize);
    munmap(pagereaded, config->dm_maxfilesize);


    close(fd);
    free(path);
}

void WritePage(PageId* pageid,unsigned char* buff)
{
    char* path = getPageIdFile(pageid);
    int fd = open(path,O_RDWR);
    if (fd == -1)
    {
        perror("Error setting file size in WritePage");
        free(path);
        return;
    }

    char* pagereaded = mmap(NULL,config->dm_maxfilesize,PROT_WRITE,MAP_SHARED,fd,0);
    if (pagereaded == MAP_FAILED)
    {
        perror("Error mmap on Write");
        free(path);
        return;
    }

    memcpy(pagereaded + pageid->PageIdx * config->pagesize,  buff, config->pagesize);
    free(path);
}


void SaveState()
/*
* Includes:
*   <stdio.h> [sizeof(); sizeof]
*   <stddef.h> [NULL; size_t]
*   <unistd.h> [chdir(); write(); close()]
*   <fcntl.h> [open()]
*   <fcntl-linux.h> [O_WRONLY; O_CREAT; O_TRUNC]
*   <assert.h> [assert()]
*
*   "PageId.h" [PageIdList; PageId; addTable()]
*   "DBConfig.h" [DBConfig]
*   "DiskManager.h" [DiskManager; count_page_id()]
* Params:
*   None.
* Return:
*   None.
* Description:
*   This function saves the state of DiskManager to dm.save.
*   This will not need page pageidlist, it will be recreated. at LoadState().
* Malloc:
*   None.
* Notes:
*   None.
*/
{
    //Move to bdd folder to open dm.save and commeback.
    chdir(config->dbpath);
    int fd = open("dm.save",O_WRONLY | O_CREAT | O_TRUNC, 0640);
    char* currentdir = currentDirectory();
    chdir(currentdir);
    free(currentdir);
    if (fd == -1)
    {
        perror("Error setting file size in SaveState");
        return;
    }

    //Count PageId and checks size
    int nb_alloc = count_page_id(diskManager->allocated, diskManager->size);
    int nb_dealloc = count_page_id(diskManager->desalocated, diskManager->size);
    assert(nb_alloc + nb_dealloc == pageidlist->size);

    //Set a PageId size
    size_t sizeof_page_id = sizeof(PageId);

    //Writes the number of PageId
    write(fd, &nb_alloc, sizeof nb_alloc);
    //Writes each Pages,
    for (int i = diskManager->size - 1; i >= 0; i--)
    {
        if (diskManager->allocated[i] == NULL)
            continue;

        write(fd, diskManager->allocated[i], sizeof_page_id);
    }

    //Writes the number of PageId
    write(fd, &nb_dealloc, sizeof nb_dealloc);
    //Writes each Pages,

    for (int i = diskManager->size - 1; i >= 0; i--)
    {
        if (diskManager->desalocated[i] == NULL)
            continue;

        write(fd, diskManager->desalocated[i], sizeof_page_id);
    }

    close(fd);
}

void LoadState()
/*
* Includes:
*
*   <unistd.h> [chdir(); read(); close()]
*   <fcntl.h> [open()]
*   <fcntl-linux.h> [O_RDONLY]
*   <stdlib.h> [free(); calloc(); realloc()]
*   <stdio.h> [perror(); sizeof]
*   <stddef.h> [NULL]
*   <string.h> [memset()]
*
*   "PageId.h" [PageIdList; PageId; addTable()]
*   "DBConfig.h" [DBConfig]
*   "DiskManager.h" [DiskManager; count_page_id()]
* Params:
*   None.
* Return:
*   None.
* Description:
*   This function loads the state of DiskManager to dm.save.
*   This will re created pageidlist, and diskManager.
* Malloc:
*   None.
* Notes:
*   None.
*/
{
    //Move to bdd folder to open dm.save and commeback.
    chdir(config->dbpath);
    int fd = open("dm.save",O_RDONLY);
    char* currentdir = currentDirectory();
    chdir(currentdir);
    free(currentdir);
    if (fd == -1)
    {
        if (errno != ENOENT)
            perror("Initialisation of default dependencies, because for dm.save"); //Good error
        return;
    }
    //return;

    //Get number of PagedId to read for allocated.
    int nb_alloc;
    read(fd, &nb_alloc, sizeof nb_alloc);

    //SETUP ALLOCATED
    //Read all the allocated pages.
    PageId **allocated = calloc(nb_alloc, sizeof *allocated);
    for (int i = nb_alloc - 1; i >= 0; i--) {
        allocated[i] = calloc(1, sizeof *allocated[i]);
        read(fd, allocated[i], sizeof *allocated[i]);
    }

    //Get number of PagedId to read for desalocated.
    int nb_dealloc;
    read(fd, &nb_dealloc, sizeof nb_dealloc);

    //Read all the desalocated pages to fill other spots with NULL.
    int nb_pageid = nb_alloc + nb_dealloc;
    void *tmp = realloc(allocated,nb_pageid * sizeof *allocated);
    if (tmp == NULL)
    {
        perror("Error realloc");
        return;
    }
    allocated = tmp;
    memset(allocated + nb_alloc , 0, (nb_pageid - nb_alloc) * sizeof *allocated);


    //SETUP DEALLOCATED
    //Read all the deallocated pages.
    PageId **deallocated = calloc(nb_pageid, sizeof *deallocated);
    for (int i = nb_dealloc - 1; i >= 0; i--) {
        deallocated[i] = calloc(1, sizeof(PageId));
        read(fd, deallocated[i], sizeof(PageId));
    }

    //SETUP pageidlist
    pageidlist = calloc(1, sizeof *pageidlist);
    pageidlist->size = nb_pageid;
    pageidlist->list = calloc(nb_pageid, sizeof *pageidlist->list);


    //Setup list of files.
    int* files = calloc(nb_pageid, sizeof *files);
    for (int i = 0; i < nb_pageid; i++)
        files[i] += -1;
    int nb_file = 0;


    //Fills pageidlist with allocated PageId and count files.
    int i = 0;
    for (int j = nb_pageid - 1; j >= 0; j--)
    {
        if (allocated[j] == NULL)
            continue;
        pageidlist->list[i++] = allocated[j];
        int file = allocated[j]->FileIdx;

        int found = 0;
        for (int k = 0; k < nb_file && !found; k++){
            if (file == files[k])
                found = 1;
        }

        if (!found)
            files[nb_file++] = file;
    }

    //Fills pageidlist with allocated PageId and count files.
    for (int j = nb_pageid - 1; j >= 0; j--)
    {
        if (deallocated[j] == NULL)
            continue;

        pageidlist->list[i++] = deallocated[j];

        int file = deallocated[j]->FileIdx;

        int found = 0;
        for (int k = 0; k < nb_file && !found; k++){
            if (file == files[k])
                found = 1;
        }

        if (!found)
            files[nb_file++] = file;
    }

    free(files);

    //Update pageidlist and diskManager.
    pageidlist->numnerofFiles = nb_file;
    diskManager->allocated = allocated;
    diskManager->desalocated = deallocated;
    diskManager->size = nb_pageid;


    close(fd);
    //No need to initiate structs since it's done there.
    config->need_init = 0;
}

PageId *FindPageId(PageId pageId)
{
    for (int i = 0; i < pageidlist->size; i++)
        if (pageId.FileIdx == pageidlist->list[i]->FileIdx && pageId.PageIdx == pageidlist->list[i]->PageIdx)
            return pageidlist->list[i];
    return NULL;
}