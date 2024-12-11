#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stddef.h"

#include "Tools_L.h"

char* currentDirectory()
/*
* Includes:
*   <limits.h> [PATH_MAX]
*   <stdlib.h> [malloc(); free()]
*   <stdio.h> [sizeof(); perror()]
*   <unistd.h> [getcwd()]
* Params:
*   None
* Return:
*  char*
*       Returns the path of the current directory. It has been malloc.
* Description:
*   This function returns the path of the current directory. This value has to be freed.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   None
*/
{

    char *cwd = (char *)malloc(PATH_MAX * sizeof(char));
    if (cwd == NULL)
    {
        perror("Failed to allocate memory for current directory");
        return NULL;
    }

    // Get the current working directory
    if (getcwd(cwd, PATH_MAX) != NULL)
    {
        return cwd; // Return the dynamically allocated path
    }
    else
    {
        perror("getcwd() error");
        free(cwd); // Free the allocated memory on error
        return NULL;
    }
}

int createFolder(const char* foldername, const char* path)
/*
* Includes:
*   <limits.h> [PATH_MAX]
*   <stdio.h> [snprintf()]
 *  <sys/stat.h> [mkdir()]
* Params:
*   const char* foldername = Name of the folder to create.
*   const char* path = Path of the folder to create.
* Return:
*   int
*       0 => Failure, might be because the file already exists
*       -1 => Success
* Description:
*   This function create a folder with foldername in the path.
* Malloc:
*   None.
* Notes:
*   It just creates the folder.
*/
{
    // Create the new path of the new folder
    char newFolderPath[PATH_MAX];
    snprintf(newFolderPath, PATH_MAX, "%s/%s", path, foldername);

    int mk2 = mkdir(newFolderPath,0755);
    if (mk2>=0)
    {
        return 0; //Sucess
        //wprintf(L"Folder created: %ls\n", newFolderPath);
    }
    else
    {
        return -1; //Failure
        //wprintf(L"Failed to create folder: %ls\n", newFolderPath); // Files already exists then
    }
}

int folderExists(const char* folderPath, const char* folderName)
/*
* Includes:
*   <dirent.h> [DIR; opendir(); closedir()]
*   <errno.h> [ENOENT; errno]
* Params:
*   const CHAR* folderPath = The path to the parent folder where you want to check for the existence of folderName.
*   const CHAR* folderName = The name of the folder you want to check for existence within the folderPath.
* Return:
*   int
*       1 => The folder exists within the specified 'folderPath'.
*       0 => The folder does not exist within the specified 'folderPath'.
*      -1 => Error of some sort.
* Description:
*   This function checks if a folder with the name 'folderName' exists within the directory specified by 'folderPath'.
*   If the folder exists, it returns 1; otherwise, it returns 0. If something is weird, returns -1.
* Malloc:
*   None.
* Notes:
*   None.
*/
{
    DIR* dir = opendir(folderName);
    if(dir)
    {
        closedir(dir);
        return 1;
    }
    else if(ENOENT == errno)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

char* readData(char* filename, int lineNumber)
/*
* Includes:
*   <stddef.h> [size_t; ();()]
*   <errno.h> [ENOENT; errno]
*   <stdio.h> [getline()]
*   <string.h> [strlen()]
*   <string.h> [strlen()]
*   <FILE.h> [FILE]
*
*  "Tools_L" [openFile; closefile()]
* Params:
*   char* filename = The name of the file to read, this includes the extension of the file. {Ex: "baka.ass"}
*   int lineNumber = The line to read, This includes - as the first line.
* Return:
*   char*
 *      Returns the whole data as a char*. This value has to be free().
* Description:
*   This function returns the line from lineNumber in the file named filename.
*   This will read line 0 as first line.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   None.
*/
{
    FILE* file = openFile(filename);
    //rl_instream = file; //Stream file

    char* line = NULL;
    size_t alloc_size = 0;
    ssize_t len;

  for (int i=-1;i<lineNumber;i++)
  //for (int i=0;i<lineNumber;i++)
  {
      if ((len = getline(&line,&alloc_size,file)) == -1)
      {
          free(line);
          closeFile(file);
          return strdup("");
      }
      //readline(""); // Read a line and in a for it goes nextline
  }

  /*
  char* lineContent = readline("");
  if(lineContent==NULL)
  {
  return NULL;
  }
  lineContent = strdup(lineContent);
  rl_instream = stdin; //Stream stdin back
  */
    line[len - 1] = 0;
    closeFile(file);
    //return lineContent;
    return line;
}

int writeDataNONL(char* filename, int lineNumber,char* content) //No need for \n
/*
* Includes:
*   <stdio.h> [getc(); EOF; fseek(); SEEK_END; SEEK_SET; SEEK_END; ftell(); perror(); fread(); sizeof(); fileno(); fwrite()]
*   <stddef.h> [NULL; size_t]
*   <stdlib.h> [malloc(); free()]
*   <string.h> [strncpy(); strlen()]
*   <unistd.h> [ftruncate()]
*   <FILE.h> [FILE]
*
*  "Tools_L" [openFile; closefile()]
* Params:
*   char* filename = The name of the file to write into.
*   int lineNumber = The line to wrtite in.
*   char* content = The content to write. This is without the \n, the function will add it in the file.
* Return:
*   int
*      0 =  Flawless execution. No error.
*      2 = Failed to load the file.
*      4 = Failed to write final content
*      5 = Failed to write in the file.
* Description:
*   This writes content at lineNumber of the filename, this does not require content to have an \n.
*   This return an execution value.
* Malloc:
*   None.
* Notes:
*   It does not remove content. So writing on a non-empty line will add content and not replace it.
*/
{
    FILE* file = openFile(filename);

    int numberofline = -1;
    char line;
    //Checks if there's the right amount of newline
    for (line = getc(file); line != EOF; line = getc(file))
    {
        if (line == '\n') // Increment count if this character is newline
            numberofline++;
    }
    //Fill remaining lines if needed
    if(lineNumber>numberofline)
    {
        char* lineToFill ="\n";
        fseek(file,0,SEEK_END);

        for(int i=0;i<lineNumber-numberofline;i++)
        {
            size_t written = fwrite(lineToFill, sizeof(char), strlen(lineToFill), file);
        }
    }

    //Write content in the rightful place
    int lineposition=0;
    fseek(file,0,SEEK_SET);

    while(lineposition<lineNumber)
    {
        if(getc(file)=='\n')
        {
            lineposition++;
        }
    }
    //Actual write
    //Step 1 Load
    //Get size of file
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *filedemat = (char*)malloc(filesize + 1);  // +1 pour le caractère nul '\0'
    if (filedemat == NULL)
    {
        perror("Failed to demat the file.");
        free(filedemat);
        closeFile(file);
        return 2;
    }
    fread(filedemat, 1, filesize, file);
    filedemat[filesize] = '\0';

    //Step 2 Replace \n in the line to edit with the context(In memory side)
    //Find the right \n
    lineposition = 0;
    int posipi = 0;
    while(lineposition<lineNumber)
    {
        if(filedemat[posipi]=='\n')
        {
            lineposition++;
        }
        posipi++;
    }

    //Split
    //Init
    size_t split1_len = posipi;
    size_t split2_len = strlen(filedemat) - posipi;
    char *split1 = malloc((split1_len + 1) * sizeof(char));
    char *split2 = malloc((split2_len + 1+1) * sizeof(char));
    //Split and fill
    strncpy(split1, filedemat, split1_len);
    split1[split1_len] = '\0';

    strcpy(split2, &filedemat[posipi]);
    if(filedemat[strlen(filedemat)])
    {
    split1[split2_len-1] = '\n';
    split1[split2_len-1] = '\0';
    }

//    //Resize content
//
//    size_t content_len = strlen(content);
//    char* contentwithNL =  malloc((content_len + 1) * sizeof(char));
//    if (contentwithNL == NULL)
//    {
//        perror("Error duplicating content");
//        free(filedemat);
//        free(split1);
//        free(contentwithNL);
//        free(split2);
//        return 3;
//    }
//    strcpy(contentwithNL, content);
//    contentwithNL[content_len] = '\n';

    //FUSION ... HA
    //size_t contentFinal_len = strlen(split1) + strlen(contentwithNL) + strlen(split2);
    size_t contentFinal_len = strlen(split1) + strlen(content) + strlen(split2);

    char *contentFinal = malloc((contentFinal_len + 1) * sizeof(char));
    if (contentFinal == NULL)
    {
        perror("Error contentFinal");
        free(filedemat);
        free(split1);
        //free(contentwithNL);
        free(split2);
        free(contentFinal);
        closeFile(file);
        return 4;
    }
    strcpy(contentFinal, split1);
    strcat(contentFinal, content);
    strcat(contentFinal, split2);


    //Step 3 Empty the file with truncate

    if (ftruncate(fileno(file), 0) != 0)
    {
        perror("Error in the rewriting on the file");
        free(filedemat);
        free(split1);
        //free(contentwithNL);
        free(split2);
        free(contentFinal);
        closeFile(file);
        return 5;
    }


    //Step 4 Fill back the file

    fseek(file, 0, SEEK_SET);
    fwrite(contentFinal, 1, strlen(contentFinal), file);

    //Step 5 return the errors

    free(filedemat);
    free(split1);
    //free(contentwithNL);
    free(split2);
    free(contentFinal);
    closeFile(file);
    return 0;
}

FILE* openFile(char* filename)
/*
* Includes:
*   <stdio.h> [fopen(); perror()]
*   <stddef.h> [NULL]
*   <FILE.h> [FILE]
* Params:
*   char* filename = The name of the file to open.
* Return:
*   FILE*
*       Returns an OPENED file. This file has to be closed with closeFile().
* Description:
*   This function open a file using a given filename.
* Malloc:
*   None.
* Notes:
*    Opens the file with "r+" mode.
*    The opened file has to be closed with closeFile().
*/
{
    FILE *file = fopen(filename, "r+");
    if (file == NULL)
    {
        perror("Error opening file %s");
        return NULL;
    }
    return file;
}

char* convertIntToChar(int var)
/*
* Includes:
*   <stdio.h> [sprintf()]
*   <stddef.h> [NULL]
*   <stdlib.h> [malloc()]
* Params:
*   int var = The int value to convert to a char*.
* Return:
*   char* => The char* converted from var.
* Description:
*   This function converts and int to a char*. This value has to be free().
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   None.
*/
{
    char *str = malloc(12);
    if (str == NULL)
    {
        return NULL;
    }
    sprintf(str, "%d", var);
    return str;
}

char* pathExtended(char* path, char* name, int isFolder)
/*
* Includes:
*   <stdio.h> [sprintf(); sizeof(); fprintf(); stderr]
*   <stddef.h> [NULL]
*   <stdlib.h> [malloc()]
*   <string.h> [strlen(); strcpy(); strcat()]
* Params:
*   char* path = Path of the folder to extend to a folder or file, mame.
*   char* name = Name of the folder or file.
*   int isFolder = 1 for folders, 0 for files,
* Return:
*   char*=>The new char* of the extended path. The return is a malloc VALUE TO FREE.
* Description:
*   This function extends the path of path to name. This does not free path, The return is a malloc VALUE TO FREE.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   This does not free path.
*/
{
    // Final path + '/' + name '/' + '\0'
    int addSlash = (path[strlen(path) - 1] != '/') ? 1 : 0;  // Savoir si on ajoute '/'
    int newLength = strlen(path) + strlen(name) + addSlash + 1; // +1 pour '\0'

    // Folder case space for final '/'
    if (isFolder)
        newLength += 1;

    char* newPath = (char*)malloc(newLength * sizeof(char));
    if (!newPath)
    {
        fprintf(stderr, "Erreur d'allocation de mémoire.\n");
        return NULL;
    }

    // New path
    strcpy(newPath, path);   // Copier le chemin de base dans newPath
    if (addSlash)
    {
        strcat(newPath, "/");
    }
    // add extentension
    strcat(newPath, name);

    // Folder case '/'
    if (isFolder && newPath[strlen(newPath) - 1] != '/')
        strcat(newPath, "/");

    return newPath;
}

char* createFileInDirectory(const char* directoryPath, const char* fileName,char* extensionwithdot)
/*
* Includes:
*   <stddef.h> [NULL]
*   <stdio.h> [fopen(); fclose(); printf()]
*   <string.h> [strlen(); strcpy(); strcat(); strdup()]
*   <FILE.h> [FILE]
* Params:
*  const char* directoryPath = The path of the direectory where the file has to be created.
*  const char* fileName = Name of the file to create (It's extension is to give int extensionwithdot field. {Ex: "Hakuna"}
*  char* extensionwithdot = The extension of the file to create. this char* has to include the dot. {Ex: ".matata"}
* Return:
*   char*
*      Returns the path of the file created.
*      Returns NULL if the file failed to be created.
* Description:
*   This function creates a file with the extensionwitdot extension in the specified directory.
*   The return value has to be free.
* Malloc:
*   The return is a malloc VALUE TO FREE.
* Notes:
*   The return is a malloc VALUE TO FREE.
*   The extension is separated from the fileName and the dot has to be included in extensionwithdot.
*/
{
    // Buffer to hold the full path
    char filePath[4096];  // 4096 is a common max path length in Linux

    // Initialize the filePath with the directoryPath
    strcpy(filePath, directoryPath);

    // Append a '/' if directoryPath does not end with it
    if (strlen(directoryPath) > 0 && directoryPath[strlen(directoryPath) - 1] != '/')
    {
        strcat(filePath, "/");
    }

    // Append the fileName and ".txt" extension
    strcat(filePath, fileName);
    strcat(filePath, extensionwithdot);

    // Create the file
    FILE *file = fopen(filePath, "w");
    if (file != NULL)
    {
        fclose(file);
        return strdup(filePath);  // Duplicate the file path to return
    } else
    {
        printf("Failed to create the file: %s\n", filePath);
        return NULL;
    }
}

char* StringandNumbers(const char* prefix, int number)
/*
* Includes:
*   <stdio.h> [printf(); sprintf()]
*   <string.h> [strlen()]
*   <stdlib.h> [malloc()]
*   <stddef.h> [NULL]
* Params:
*   const char* prefix = A constant string with the prefix letters (e.g., "TP").
*   int number = The integer number to append to the prefix.
* Return:
*   char* => The new string with the format "prefix+number" (e.g., "TP1", "TP29").
* Description:
*   This function creates a new string that starts with the given prefix and
*   appends the integer value as a string to it.
* Notes:
*   The returned string should be freed by the caller after use.
*/
{
        // Calculate the length needed for the resulting string
        // The length includes the prefix length, the number converted to string, and the null terminator
        int length = strlen(prefix) + 10; // 10 characters are enough to handle any int

        // Allocate memory for the new string
        char* result = (char*)malloc(length * sizeof(char));
        if (result == NULL)
        {
            printf("Memory allocation failed\n");
            return NULL;
        }

        // Create the final string using sprintf
        sprintf(result, "%s%d", prefix, number);

        return result;  // Return the new dynamically allocated string
}

int remove_directory(const char *path)
/*
* Includes:
*   <dirent.h> [DIR; opendir(); dirent; readdir(); closedir()]
*   <string.h> [strlen(); strcmp()]
*   <stdio.h> [snprintf()]
*   <stdlib.h> [malloc(); free()]
*   <stddef.h> [size_t]
*   <stat.h> [stat; S_ISDIR(); statbuf()]
*   <unistd.h> [unlink(); rmdir()]
*
*   "Tools_L.c" [remove_directory()]
* Params:
*   const char* path = The path of the folder to delete.
* Return:
*   int
 *      0 = Success.
 *      -1 = Failure.
* Description:
*   This function deletes a file from path and everything inside.
* Notes:
*   None.
*/
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;
        r = 0;

        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            // Ignore les entrées "." et ".."
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);

    return r;
}