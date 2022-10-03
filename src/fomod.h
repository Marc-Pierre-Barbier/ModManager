#ifndef __FOMOD_H__
#define __FOMOD_H__

#include <stdbool.h>
#include <glib.h>
#include "fomod/parser.h"

#include "fomod/group.h"
#include "fomod/xmlUtil.h"

/**
 * @brief Start a terminal based install process for fomod.
 *
 * @param modFolder folder of the fomod file.
 * @param destination folder of the new mod that contains the result of the fomod process.
 * @return int
 */
int installFOMod(const char * modFolder, const char * destination);

/**
 * @brief In fomod there is file operations which depends on multiple flags this function find the ones that mach our current flags and append them to a list.
 *
 * @param fomod a pointer to a fomod obtained by the parser
 * @param flagList a FOModFlag_t list which contains all of the flag found.
 * @param pendingFileOperations a list of pending FOModFile_t operation to which add the new ones. (can be null)
 * @return a list of pendingFileOperations(FOModFile_t)
 */
GList * processCondFiles(const FOMod_t * fomod, GList * flagList, GList * pendingFileOperations) __attribute__((warn_unused_result));

/**
 * @brief FOModFile_t have a priority option and this function execute the file operation while taking this into account.
 *
 * @param pendingFileOperations list of FOModFile_t to process
 * @param modFolder folder of the fomod file.
 * @param destination folder of the new mod that contains the result of the process.
 * @return error code
 */
int processFileOperations(GList * pendingFileOperations, const char * modFolder, const char * destination);



#endif
