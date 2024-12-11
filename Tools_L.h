#ifndef SHINBDDA_TOOLS_L_H
#define SHINBDDA_TOOLS_L_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Structures.h"

#ifdef __cplusplus
extern "C" {
#endif

char* currentDirectory();
int createFolder(const char* foldername, const char* path);
int folderExists(const char* folderPath, const char* folderName);

static inline int fileExists(const char* path)
/*
 * Includes:
 *      <unistd.h> [access(); F_OK]
 * Params:
 *      const char* path = path of the file.
 * Return:
 *   int
 *       1 => The folder exists within the specified 'folderPath'.
 *       0 => The folder does not exist within the specified 'folderPath'.
 *      -1 => Error of some sort.
 * Description:
 *      This function checks if the file at path exists.
 *   If the folder exists, it returns 1; otherwise, it returns 0. If something is weird, returns -1.
 * Notes:
 *      None.
 */
{
    if(access(path, F_OK) == 0)
    {
        return 1;
    } else {
        return 0;
    }
    return -1;
}
char* readData(char* filename, int lineNumber);
int writeDataNONL(char* filename, int lineNumber,char* content);
FILE* openFile(char* filename);

static inline int closeFile(FILE* file)
/*
 * Includes:
 *      <stdio.h> [fclose()]
 *      <FILE.h> [FILE]
 * Params:
 *      File* files = The file to close.
 * Return:
 *  int
 *      -1=> Success
 *      0 => Failure
 * Description:
 *      This function closes a give file and returns exec code.
 * Notes:
 *      None.
 */
{
    return fclose(file);
}

char* convertIntToChar(int var);

static inline int convertChartoInt(char* var)
/*
 * Includes:
 *      <stdlib.h> [atoi()]
 * Params:
 *      char* var = The string to convert to int.
 * Return:
 *  int
 *      The int converter var String.
 * Description:
 *      This function converts a string to int and returns it.
 * Notes:
 *      None.
 */
{
    return atoi(var);
}

char* pathExtended(char* path, char* name, int isFolder);

static inline int checkDATA(char* filename, int lineNumber) //return 1 if existing,0 Otherwise
/*
 * Includes:
 *      <string.h> [strcmp()]
 *      <stdlib> [free()]
 *
 *      "Tools_L.h"[readData()]
 * Params:
 *      File* files = The file to close.
 * Return:
 *  int
 *      1=> Success
 *      0 => Failure
 * Description:
 *      This function closes a give file and returns exec code.
 * Notes:
 *      None.
 */
{
    char* data = readData(filename, lineNumber);
    int returnvalue = (data == NULL || strcmp(data, "") == 0) ? 0 : 1;
    free(data);
    return returnvalue;
}

char* createFileInDirectory(const char* directoryPath, const char* fileName,char* extensionwithdot);
char* StringandNumbers(const char* prefix, int number);
int remove_directory(const char *path);

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_TOOLS_L_H
