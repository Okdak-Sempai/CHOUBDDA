#ifndef SHINBDDA_DBCONFIG_H
#define SHINBDDA_DBCONFIG_H

#include "Structures.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Policy {
    POLICY_MRU,
    POLICY_LRU,
} Policy;

typedef struct DBConfig
/*
*   Configurations data
*/
{
    char* configpath; // Settings config path.
    char* dbpath; // Le dossier BDD Stack //A MALLOC
    int pagesize;// Tailles d'une page
    int dm_maxfilesize; // Tailles Max d'un fichier rsdb
    int dm_buffercount; // Number of BufferManager to manage
    Policy dm_policy; // Replacement policy(LRU or MRU)
    uint8_t need_init; // If it needs Initialisation of if it reads saved state.
} DBConfig;

extern DBConfig  *config;

// void constructDBConfig(char* configpath, char* dbpath, int pagesize, int dm_maxfilesize, int dm_buffercount, Policy dm_policy, uint8_t need_init);
void LoadDBConfig(const char* fichier_config);
// char* defaultBDDStackSetter(char* filename);
// void settingsSetter(char *settingsfilename);
// char* settingsInit(char* settingsname);
// int restoreDefault(char* settingsname);
// void ConfigInit();
void DBFree();

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_DBCONFIG_H
