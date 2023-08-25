#ifndef __PUBLIC_FOMOD_H__
#define __PUBLIC_FOMOD_H__

#include "errorType.h"
#include <glib.h>
#include <gio/gio.h>
#include "fomodTypes.h"

/**
 * @brief parse a fomod xml file (found inside the mod folder /fomod/moduleconfig.xml (everything should be lowercase))
 *
 * @param fomodFile path to the moduleconfig.xml
 * @param fomod pointer to a new FOMod_t
 */
error_t fomod_parse(GFile * fomod_file, Fomod_t* fomod);

/**
 * @brief In fomod there is file operations which depends on multiple flags this function find the ones that mach our current flags and append them to a list.
 *
 * @param fomod a pointer to a fomod obtained by the parser
 * @param flagList a FOModFlag_t list which contains all of the flag found.
 * @param pendingFileOperations a list of pending FOModFile_t operation to which add the new ones. (can be null)
 * @return a list of pendingFileOperations(FOModFile_t)
 */
GList * fomod_process_cond_files(const Fomod_t * fomod, GList * flag_list, GList * pending_file_operations) __attribute__((warn_unused_result));

/**
 * @brief FOModFile_t have a priority option and this function execute the file operation while taking this into account.
 *
 * @param pendingFileOperations list of FOModFile_t to process
 * @param modFolder folder of the fomod file.
 * @param destination folder of the new mod that contains the result of the process.
 * @return error code
 */
error_t fomod_process_file_operations(GList ** pending_file_operations, GFile * mod_folder, GFile* destination);

/**
 * @brief
 *
 * @param fileOperations
 */
void fomod_free_file_operations(GList * file_operations);


/**
 * @brief Free content of a fomod file.
 * @param fomod
 */
void fomod_free_fomod(Fomod_t * fomod);

#endif
