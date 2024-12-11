#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Tools_L.h"
#include "DBConfig.h"

void constructDBConfig(char* configpath, char* dbpath, int pagesize, int dm_maxfilesize, int dm_buffercount, Policy dm_policy, uint8_t need_init)
/*
* Includes:
*   <stdio.h> [sizeof()]
*
*   "DBConfig.h" [DBConfig]
* Params:
*  char* configpath = The path of the configuration file.
*  char* dbpath = The path of the BDD folder.
*  int pagesize = The Size of a Page(byte).
*  int dm_maxfilesize = The maximum size of a file that containes Pages.
*  int bm_buffercount = The number of BufferManager to manage.
*  char* bm_policy = Replacement Policy. ONLY LRY OR MRU
*  uint8_t need_init = The flag for values loading or initatilsation[0/1].
* Return:
*   DBConfig*
*      Returns the DBConfig struct initialized.
* Description:
*   This function creates and returns the DBConfig structure. This has to be free() later on.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   config->dbpath has to be free() later on. VALUE TO FREE.
*/
{
    config = (DBConfig*) malloc(sizeof(DBConfig));
    if (config == NULL) {
        perror("error: malloc config:");
        abort();
    }

    config->configpath = configpath;// Settings config path.
    config->dbpath = dbpath; // Path of bdd
    config->pagesize = pagesize; // Taille d'une page Octets
    config->dm_maxfilesize = dm_maxfilesize; // Taille max d'un fichier Octets
    config->dm_buffercount = dm_buffercount; // Number of BufferManager to manage
    config->dm_policy = dm_policy; // Replacement Policy.
    config->need_init = need_init; // If it needs Initialisation of if it reads saved state.
}

void LoadDBConfig(char* fichier_config)
/*
* Includes:
*   <stdlib.h> [free()]
*
*   "Tools_L.h" [readData(); convertChartoInt()]
*   "DBConfig.h" [DBConfig]*
Params:
*   char* fichier_config = Name of the configuration file to read and load the data.
* Return:
*   DBConfig*
*      Returns the DBConfig struct initialized with the fichier_config value.
* Description:
*   This function loads fichier_config to read its data and creates a DBConfig struct from it, then returns it.
*   DBConfig* has to be free().
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   config->dbpath has to be free() later on. VALUE TO FREE.
*   Checks if the config file has either LRU or MRU, if invalid value, uses LRU.
*/
{
    char* configpath = readData(fichier_config,1);

    char* dbpath = readData(fichier_config,2);

    char* charpagesize = readData(fichier_config,3);
    int pagesize = convertChartoInt(charpagesize);

    char* chardm_maxfilesize = readData(fichier_config,4);
    int dm_maxfilesize = convertChartoInt(chardm_maxfilesize);

    char* chardm_buffercount = readData(fichier_config,5);
    int dm_buffercount = convertChartoInt(chardm_buffercount);

    char* raw_policy = readData(fichier_config,6);
    Policy dm_policy;

    if (strcmp(raw_policy, "LRU") == 0)
        dm_policy = POLICY_LRU;
    else if (strcmp(raw_policy, "MRU") == 0)
        dm_policy = POLICY_MRU;
    else
        dm_policy = POLICY_LRU;
    free(raw_policy);

    char* charneed_init = readData(fichier_config,7);
    uint8_t need_init = convertChartoInt(charneed_init);

    constructDBConfig(configpath,dbpath,pagesize,dm_maxfilesize,dm_buffercount,dm_policy,need_init);


    //free(dbpath); Don't free that man,
    free(charpagesize);
    free(chardm_maxfilesize);
    free(chardm_buffercount);
    free(charneed_init);
    //return NULL;
}


char* defaultBDDStackSetter(char* filename)
/*
* Includes:
*   <stdlib.h> [free()]
*
*   "Tools_L.h" [currentDirectory(); createFolder(); folderExists(); readData()]
*   "DBConfig.h" [DBConfig]*
Params:
*   char* filename = The name of the config file.
* Return:
*   char* => Returns the path of the BDD Folder.
* Description:
*   This function creates the BDD STack(The BDD folder).
*   This writes the folder's address on the line 2 of the file(so the 3rd line) and returns this address.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   None.
*/
{
    int BDDStacksettingsline = 2;
    //Create BDD Stack
    char* currentDir = currentDirectory();
    char* bddpath = pathExtended(currentDir,"BDD Stack",1);
    if (!(folderExists(currentDir, "BDD Stack")))
    {
        createFolder("BDD Stack", currentDir);
        writeDataNONL(filename,BDDStacksettingsline,bddpath);// Write in settings

        //Creates BinData in BDD Stack
        if(!folderExists(bddpath,"BinData"))
        {
            createFolder("BinData",bddpath);
        }

        //free(bddpath);
        free(currentDir);
        return bddpath; // The file didn't exist and has been created
    }


    //Creates BinData in BDD Stack
    if(!folderExists(bddpath,"BinData"))
    {
        createFolder("BinData",bddpath);
    }
    free(bddpath);
    free(currentDir);
    return readData(filename,BDDStacksettingsline); // The file already exists
}

void settingsSetter(char *settingsfilename)
/*
* Includes:
*   <stdlib.h> [free()]
*
*   "Tools_L.h" [convertIntToChar(); checkDATA(); writeDataNONL()]
*   "DBConfig.h" [DBConfig; defaultBDDStackSetter(); LoadDBConfig()]
Params:
*   char* settingsfilename = The name of the config file.
* Return:
*   DBConfig* => Returns the BDConfig structure.
* Description:
*   This function checks if the settings have values for DBConfig*.
*   If there is none it will write default values
*   It will load the values from the settings files, create and return a DBConfig* config. This will have to be free().
*   Values have to be check in decreasing order in case there's not enough '\n'.
*
*   There is 7 Values.
*      char* nned_init is set to 1 on line 7.
*      char* bm_policy is set to LRU but can be MRU on line 6.
*      int bm_buffercount is 4 on line 5.
*      char* dm_maxfilesize is 3*4096 on line 4.
*      char* pagesize is 4096 on line 3.
*      char* settingspath is created the BDD Folder if non-existent on line 2.
*      char* configpath is the default config path on line 1.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   Creates BDD Stack folder.
*   config->dbpath has to be free() later on. VALUE TO FREE.
*/
{
    int numberofARGS = 7;

    //need_init
    char* need_init = convertIntToChar(1); // To free
    int tmpinit = checkDATA(settingsfilename,7); //Looks necessary
    if(!checkDATA(settingsfilename,7))
    { writeDataNONL(settingsfilename,7, need_init);}

    //dm_policy
    char* dm_policy = "LRU"; // LRU OR MRU ONLY
    if(!checkDATA(settingsfilename,6))
    { writeDataNONL(settingsfilename,6,dm_policy);}

    //dm_buffercount
    char* dm_buffercount = convertIntToChar(4); //To free
    int tmpdm_buffercount = checkDATA(settingsfilename,5); //Looks necessary
    if(!checkDATA(settingsfilename,5))
    { writeDataNONL(settingsfilename,5, dm_buffercount);}

    //dm_maxfilesize
    char* dm_maxfilesize = convertIntToChar(3*4096); // Byte
    int tmp = checkDATA(settingsfilename,4); //Looks necessary
    if(!checkDATA(settingsfilename,4))
    { writeDataNONL(settingsfilename,4, dm_maxfilesize);}

    //pagesize
    char* pagesize = convertIntToChar(4096); //To free
    if(!checkDATA(settingsfilename,3))
    { writeDataNONL(settingsfilename,3, pagesize);}

    //bddpath
    char* settingspath = defaultBDDStackSetter(settingsfilename); //Creates BDD Stack folder
    if(!checkDATA(settingsfilename,2))
    { writeDataNONL(settingsfilename,2,settingspath);}

    char* configpath = settingsInit(settingsfilename);
    if(!checkDATA(settingsfilename,1))
    { writeDataNONL(settingsfilename,1, configpath);}

    //Load, and Returns struct, EZ

    LoadDBConfig(settingsfilename);

    //Free
    free(settingspath);
    free(pagesize);
    free(dm_maxfilesize);
    free(need_init);
    free(configpath);
    free(dm_buffercount);
}

char* settingsInit(char* settingsname)
/*
* Includes:
*   <stdio.h> [sizeof(); perror(); snprintf(); fclose()]
*   <stdlib.h> [free(); malloc()]
*   <stddef.h> [NULL; size_t]
*   <string.h> [strlen()]
*   <file.h> [FILE]
*
*   "Tools_L.h" [currentDirectory()]
* Params:
*   char* settingsfilename = The name of the config file.
* Return:
*   char* => The path of the created settingsname file or already existing one.
* Description:
*   This function creates the settings file in the current folder and returns its path.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   This doesn't write in the created folder.
*/
{
    char* currentPath = currentDirectory();
    if(currentPath==NULL)
    {
        free(currentPath);
        return NULL;
    }

    size_t settingsPathLength = strlen(currentPath) + strlen(settingsname) + 2;
    char* settingsPath = (char*)malloc(settingsPathLength * sizeof(char));

    if (settingsPath == NULL)
    {
        perror("Settings path allocation error");
        free(currentPath);
        free(settingsPath);
        return NULL;
    }

    //Acutal settingsPath
    snprintf(settingsPath, settingsPathLength, "%s/%s", currentPath, settingsname);

    // Create the settings file
    FILE* file = fopen(settingsPath, "wx");
    if (file == NULL)
    {
        //perror("Error creating file");
        free(currentPath);
        return settingsPath;
    }
    fclose(file);
    free(currentPath);
    return settingsPath;
}

int restoreDefault(char* settingsname)
/*
* Includes:
*   <stdlib.h> [free()]
*
*   "Tools_L.h" [currentDirectory(); pathExtended(); folderExists(); fileExists()]
* Params:
*   char* settingsfilename = The name of the config file.
* Return:
*   0 => Success.
* Description:
*   This function delete default files so it can be re-initiate. It checks if either settings OR the BDD Folder OR dm.save is not there to launch.
* Malloc:
*   None.
* Notes:
*   None.
*/
{

    char* currentDir = currentDirectory();
    char* bddpath = pathExtended(currentDir,"BDD Stack",1);
    char* dmsave = pathExtended(bddpath,"dm.save",0);
    if(!folderExists(currentDir,"BDD Stack") || !fileExists(settingsname) || !fileExists(dmsave))
    {
        //remove(bddpath);
        remove_directory(bddpath);
        remove(settingsname);
        remove("dm.save");
    }
    free(currentDir);
    free(bddpath);
    free(dmsave);
    return 0;
}

void ConfigInit()
/*
* Includes:
*   <stdlib.h> [free()]
*
*   "Tools_L.h" [currentDirectory()]
*   "DBConfig.h" [settingsInit(); settingsSetter(); DBConfig]
* Params:
*   None.
* Return:
*   DBConfig* => Returns DBConfig* config structure.
* Description:
*   This function Initiates DBConfig* config, it creates a file with default values or uses custom values from the setting files if already existent.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   This shall be used almost first on the main to load a config or create one.
*   config->dbpath has to be free() later on. VALUE TO FREE.
*/
{
    char *currentPath = currentDirectory();
    char *settingsfilename = "fichier_config.txt";
    restoreDefault(settingsfilename);
    char *settingsPathName = settingsInit(settingsfilename);

    //Step Checks Settings existence and creates it if non existant, First line is settings address.
    settingsSetter(settingsfilename);


    free(currentPath);
    free(settingsPathName);
/*    //test
    printf(readData(settingsfilename,2));

    //null test
    char* testy = readData(settingsfilename,2);
    if(!strcmp(readData(settingsfilename,2),""))
    {
        printf("\nSucess\n");
    }
    else
    {
        printf("\nTested is [%s]",testy);
    }*/

//
//    writeDataNONL(settingsfilename,2, "tortue");
//    writeDataNONL(settingsfilename,5, "mario");
//    writeDataNONL(settingsfilename,7, "tortue+8+98+98658");
//    writeDataNONL(settingsfilename,3, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
//    writeDataNONL(settingsfilename,6, "kaka");
//    writeDataNONL(settingsfilename,4, "Mamambo");
//
//    char *a = readData(settingsfilename, 1);
//    printf("a is %s\n", a);
//    char *b = readData(settingsfilename, 2);
//    printf("b is %s\n", b);
//    char *c = readData(settingsfilename, 3);
//    printf("c is %s\n", c);
//    char *d = readData(settingsfilename, 4);
//    printf("d is %s\n", d);
//    char *e = readData(settingsfilename, 5);
//    printf("e is %s\n", e);
//    char *f = readData(settingsfilename, 7);
//    printf("f is %s\n", f);
//    char *g = readData(settingsfilename, 6);
//    printf("g is %s\n", g);
//    char *h = readData(settingsfilename, 8);
//    printf("h is %s\n", h);
//    printf("New World!\n");
//    free(a);
//    free(b);
//    free(c);
//    free(d);
//    free(e);
//    free(f);
//    free(g);
}

void DBFree()
{
    free(config->dbpath);
    free(config->configpath);
    free(config);
}