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
 * @brief In fomod there are unconditional files which are required for all options
 * @param fomod a pointer to a fomod obtained by the parser
 * @param pendingFileOperations a list of pending FomodFile_t operation to which add the new ones. (can be null)
 * @return a list of pendingFileOperations(FomodFile_t) which newly added entries
 */
GList * fomod_process_required_files(const Fomod_t * fomod, GList * pending_file_operations) __attribute__((warn_unused_result));

/**
 * @brief In fomod there is file operations which depends on multiple flags this function find the ones that mach our current flags and append them to a list.
 *
 * @param fomod a pointer to a fomod obtained by the parser
 * @param flagList a FOModFlag_t list which contains all of the flag found.
 * @param pendingFileOperations a list of pending FomodFile_t operation to which add the new ones. (can be null)
 * @return a list of pendingFileOperations(FomodFile_t)
 */
GList * fomod_process_cond_files(const Fomod_t * fomod, GList * flag_list, GList * pending_file_operations) __attribute__((warn_unused_result));

/**
 * @brief FOModFile_t have a priority option and this function execute the file operation while taking this into account.
 * this create a new mod with the __FOMOD postfix in the name
 * @param pendingFileOperations list of FOModFile_t to process
 * @param modFolder mod_name
 * @return error code
 */
error_t fomod_execute_file_operations(GList ** pending_file_operations, int mod_id, int appid);

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
