#ifndef __PUBLIC_FOMOD_H__
#define __PUBLIC_FOMOD_H__

#include "errorType.h"
#include <glib.h>
#include "fomodTypes.h"

/**
 * @brief parse a fomod xml file (found inside the mod folder /fomod/moduleconfig.xml (everything should be lowercase))
 *
 * @param fomodFile path to the moduleconfig.xml
 * @param fomod pointer to a new FOMod_t
 */
error_t parser_parseFOMod(const char * fomodFile, FOMod_t* fomod);

/**
 * @brief In fomod there is file operations which depends on multiple flags this function find the ones that mach our current flags and append them to a list.
 *
 * @param fomod a pointer to a fomod obtained by the parser
 * @param flagList a FOModFlag_t list which contains all of the flag found.
 * @param pendingFileOperations a list of pending FOModFile_t operation to which add the new ones. (can be null)
 * @return a list of pendingFileOperations(FOModFile_t)
 */
GList * fomod_processCondFiles(const FOMod_t * fomod, GList * flagList, GList * pendingFileOperations) __attribute__((warn_unused_result));

/**
 * @brief FOModFile_t have a priority option and this function execute the file operation while taking this into account.
 *
 * @param pendingFileOperations list of FOModFile_t to process
 * @param modFolder folder of the fomod file.
 * @param destination folder of the new mod that contains the result of the process.
 * @return error code
 */
error_t fomod_processFileOperations(GList ** pendingFileOperations, const char * modFolder, const char * destination);

/**
 * @brief
 *
 * @param fileOperations
 */
void fomod_freeFileOperations(GList * fileOperations);


/**
 * @brief Free content of a fomod file.
 * @param fomod
 */
void fomod_freeFOMod(FOMod_t * fomod);

#endif
