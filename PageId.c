#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "PageId.h"
#include "DiskManager.h"
#include "Tools_L.h"
#include "DBConfig.h"
#include <sys/stat.h>

PageIdList* InitiatiateTables(char* folderpath,int filesize,int pagesize, int number)
/*
* Includes:
*   <stdio.h> [perror(); sizeof()]
*   <stdlib.h> [malloc(); free()]
*   <stddef.h> [NULL; size_t]
*   <fcntl.h> [open()]
*   <fcntl-linux.h> [O_WRONLY]
*   <unistd.h> [ftruncate(); close()]
*
*   "PageId.h" [PageIdList; PageId]
*   "Tools_L.h" [pathExtended(); StringandNumbers(); createFileInDirectory()]
* Params:
*   char* folderpath = The path of the BDD Folder.(BDD Stack) //DB Path of config shall be used.
*   int filesize = The size of a file. //dm_maxfillesize of config shall be used
*   int pagesize = The size of a Page in a file. //pagesize of config shall be used.
*   int number = The number of files to create.
* Return:
*   PageIdList* => Initiate and returns the PageIdList* pageIdlisted.
*   NULL => There's been an error.
* Description:
*   This function initiates PageIdList* pageIdlisted and returns it.
 *   This creates number of files and assigned each Page to the pageIdlisted.
* Malloc:
*   The return is a malloc VALUE TO FREE. and each PageId in it has to be free()
* Notes:
*   This initiates the list of PageId and creates those files as well.
*/
{

    int numberofpages= filesize/pagesize;
    //PageIds Init
    PageIdList* pageIdlisted = (PageIdList*)malloc(sizeof(PageIdList));
    if (pageIdlisted == NULL)
    {
        perror("Memory allocation failed for PageIdList");
        return NULL;
    }

    pageIdlisted->size =0;
    pageIdlisted->numnerofFiles=0;
    size_t total_pages = (size_t)number * numberofpages;
    PageId** pageArray = (PageId**)malloc(total_pages * sizeof(PageId*));
    if (pageArray == NULL)
    {
        perror("Memory allocation failed for PageIds");
        free(pageIdlisted);
        return NULL;
    }

    pageIdlisted->list= pageArray;

    //Files creations
    char* BinDatapath = pathExtended(folderpath,"BinData",1);
    for (int i = 0; i < number; i++)
    {
        //pageIdlisted->list[i].FileIdx=i;
        for(int j=0;j<numberofpages;j++)
        {
            pageIdlisted->size++;
            int index = i * numberofpages + j;
            pageIdlisted->list[index]=(PageId*) malloc(sizeof(PageId));
            if(pageIdlisted->list[index]==NULL)
            {
                perror("Memory allocation failed for specific PageIds");
                free(pageIdlisted);
                return NULL;
            }
            pageIdlisted->list[index]->FileIdx = i;
            pageIdlisted->list[index]->PageIdx = j;
            //pageIdlisted->list[i].PageIdx =j;
        }
        pageIdlisted->numnerofFiles++;

        //Create file
        char* newfile = StringandNumbers("F",i);
        char* newfilepath = createFileInDirectory(BinDatapath,newfile,".rsdb");

        //Make a set size
        int fd = open(newfilepath, O_WRONLY);
        if (fd != -1)
        {
            // Set file size to specified size
            if (ftruncate(fd, filesize) == -1)
            {
                // Handle error: couldn't set file size
                perror("Error setting file size in InitiatiateTables");
            }
            close(fd);
        }
        else
        {
            perror("Error opening file init");
        }
        free(newfile);
        free(newfilepath);
    }
    free(BinDatapath);

    return pageIdlisted;
}

PageId* addTable(char* folderpath,int filesize,int pagesize,int toadd,PageIdList* pageIdlisted, DiskManager* diskManager)
/*
* Includes:
*   <stdio.h> [perror(); sizeof()]
*   <stdlib.h> [realloc(); free(); malloc()]
*   <stddef.h> [NULL;size_t]
*   <fcntl.h> [open()]
*   <fcntl-linux.h> [O_WRONLY]
*   <unistd.h> [ftruncate(); close()]

*
*   "PageId.h" [PageIdList; PageId]
*   "Tools_L.h" [pathExtended(); StringandNumbers(); createFileInDirectory()]
*   "DiskManager.h" [DiskManager]
* Params:
*   char* folderpath = The path of the BDD folder.(BDD Stack) //dbpath of config shall be used.
*   int filesize = The max size of each file // dm_maxfillesize of config shall be used
*   int pagesize = The size of a Page in a file. //pagesize of config shall be used.
*   int toadd = Number of Files to add. This will create (filesize / pagesize) PageId per file.
*   PageIdList* pageIdlisted = The pageIdListed.
*   DiskManager* diskManager = The diskManager.
* Return:
*   PageId* => Returns the Newer PageId. This PageId is already put in diskManager->allocated.
*   NULL => There's been an error.
* Description:
*   This function add toadd number of files to the BinData files. Each files produces (filesize / pagesize) of PageId.
*   The first ever created PageId will be returned and put in diskManager->allocated. The other PagId will be in diskManager->desalocated
*   Each new PageId is stored in PageIdList* pageIdlisted.
* Malloc:
*   The returned PageId HAS NOT TO BE free().
* Notes:
*   Always creates at least (filesize / pagesize) PageId.
*   diskManager->desalocated is filled in reverse for LIFO.
*/
{
    //Initials Index
    int Initialpagesize=pageIdlisted->size;
    int Initialdesalocatedsize=0;
    int Initialallocatedsize=diskManager->size;


    PageId* freePageId=NULL;
    int freePageFlag=0;
    char* BinDatapath = pathExtended(folderpath,"BinData",1);


    //Gestion PageID
    int numberofpages = filesize / pagesize;
    int arrival = (pageIdlisted->numnerofFiles + toadd);//Files
    int totalsize=(pageIdlisted->size) + (toadd*numberofpages);//Pages
    size_t new_total_pages = (size_t)(totalsize);
    PageId** new_pageArray = (PageId**)realloc(pageIdlisted->list, new_total_pages * sizeof(PageId*));
    if (new_pageArray == NULL)
    {
        perror("Memory reallocation failed for PageIds");
        return NULL;
    }
    pageIdlisted->list = new_pageArray;
    pageIdlisted->size= totalsize;
    diskManager->size=totalsize;

    //Gestion DiskManager
    //allocated array realloc
    PageId** new_allocatedArray = (PageId**)realloc(diskManager->allocated,new_total_pages* sizeof(PageId*));
    if(new_allocatedArray == NULL)
    {

        perror("Memory reallocation failed for new_allocatedArray");
        free(new_allocatedArray);
        return NULL;
    }
    diskManager->allocated=new_allocatedArray;
    for(int i=Initialpagesize;i<totalsize;i++)
    {
        diskManager->allocated[i]=NULL;
    }
    //desalocatd array realloc
    PageId** new_desalocatedArray = (PageId**)realloc(diskManager->desalocated,new_total_pages* sizeof(PageId*));
    if(new_desalocatedArray == NULL)
    {

        perror("Memory reallocation failed for new_desalocatedArray");
        free(new_desalocatedArray);
        return NULL;
    }
    diskManager->desalocated=new_desalocatedArray;
    //Mettre NULL dans les nouvelles cases de desalocated
    for (int i = 0; i < new_total_pages; i++)
    {
        diskManager->desalocated[i] = NULL;
    }

    //
    //Trouver l'index ou ecrire dans desalocated
    int index = Initialpagesize;
    int newsize = (arrival-pageIdlisted->numnerofFiles)*numberofpages-1-1;//-1 Pour l'index et -1 parce qu'on va mettre la premiere nouvelle page dans allocated
    //
    for (pageIdlisted->numnerofFiles; pageIdlisted->numnerofFiles < arrival; pageIdlisted->numnerofFiles +=1)
    {
        printf("Creating pages for file no %d\n", pageIdlisted->numnerofFiles);
        //GESTION PageID
        for(int j=0;j<numberofpages;j++)
        {
            pageIdlisted->list[index]=(PageId*) malloc(sizeof(PageId));
            if(pageIdlisted->list[index]==NULL)
            {
                perror("Memory allocation failed for specific realoc of PageIds");
                free(pageIdlisted);
                return NULL;
            }
            pageIdlisted->list[index]->FileIdx = pageIdlisted->numnerofFiles+1;
            pageIdlisted->list[index]->PageIdx = j;
            //pageIdlisted->list[i].PageIdx =j;
            if(j==0 && freePageFlag==0) // On ajoute a allocated
            {
                freePageId = diskManager->allocated[Initialallocatedsize]=pageIdlisted->list[index];//On ecrit direct au bon endroit
                freePageFlag++;
            }
            else//On met le reste dans desalocated
            {
                diskManager->desalocated[newsize]=pageIdlisted->list[index];
                newsize--;
            }
            index++;
        }


        //
        char* newfile = StringandNumbers("F",pageIdlisted->numnerofFiles + 1);
        char* newfilepath = createFileInDirectory(BinDatapath,newfile,".rsdb");
        //Make a set size
        int fd = open(newfilepath, O_WRONLY | O_CREAT);
        if (fd != -1)
        {
            struct stat s;
            if (fstat(fd, &s) == -1)
                perror("couldn't stat file");

            if (s.st_size != 0) {
                fprintf(stderr, "error: %s: non empty file, no ftruncate\n", newfile);
            }
            else if (ftruncate(fd, filesize) == -1) // Set file size to specified size
            {
                // Handle error: couldn't set file size
                perror("Error setting file size in addTable");
            }
            close(fd);
        }
        else
        {
            perror("Error opening file add");
        }
        free(newfile);
        free(newfilepath);
    }

    free(BinDatapath);
    return freePageId;
}

char* getPageIdFile(PageId* pageid)
/*
* Includes:
*   <stddef.h> [NULL]
*   <string.h> [strlen(); strcat()]
*   <stdlib.h> [realloc(); free()]
*
*   "PageId.h" [PageIdList; PageId]
*   "Tools_L.h" [pathExtended(),StringandNumbers]
*   "DBConfig.h" [DBConfig]
* Params:
*   DBConfig* config = The DBConfig* config
*   PageId* pageid = The PageId* pageid to get the file of.
* Return:
*   char* => Returns the path of the PageId pageid.
*   NULL => There's been an error.
* Description:
*   This function returns the path of the pagid's file.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   None.
*/
{
    char* BinDatapath = pathExtended(config->dbpath,"BinData",1);//folder name
    char* newfile = StringandNumbers("F",pageid->FileIdx); //file name

    char* newfile2 = realloc(newfile, strlen(newfile) + strlen(".rsdb") + 1);
    if (newfile2 == NULL) { // Vérifie si realloc a échoué
        free(BinDatapath);
        free(newfile);
        return NULL; // Retourne NULL en cas d'erreur
    }
    strcat(newfile2, ".rsdb");


    char* pageidpath = pathExtended(BinDatapath, newfile2, 0); // Filename


    free(BinDatapath);
    free(newfile2);
    return pageidpath;
}