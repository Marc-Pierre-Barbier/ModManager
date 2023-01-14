#ifndef __FILE_H__
#define __FILE_H__

#include <stdbool.h>
#include <glib.h>
#include <sys/types.h>

#include "constants.h"


//valid copy flags
#define FILE_CP_DEFAULT 0
#define FILE_CP_LINK 1
#define FILE_CP_RECURSIVE 2
#define FILE_CP_NO_TARGET_DIR 4

/**
 * @brief execute the cp command from the source to the dest
 * @param source path to the source files
 * @param dest path to the destination folder
 * @param flags refer to the "valid copy flags" use them like this CP_LINK | CP_RECURSIVE
 * @return cp return value
 */
int file_copy(const char * source, const char * dest, u_int32_t flags);

/**
 * @brief execute the cp command from the source to the dest
 * @param source path to the source files
 * @param dest path to the destination folder
*  @param bool enable recursive rm -r
 * @return rm return value
 */
int file_delete(const char * path, bool recursive);

/**
 * @brief Run the mv command
 * @param source
 * @param destination
 * @return mv's exit value
 */
int file_move(const char * source, const char * destination);

/**
 * @brief Recursively rename all file and folder to lowercase.
 *
 * @param folder
 */
void file_casefold(const char * folder);


const char * file_extractLastPart(const char * filePath, const char delimeter);

/**
 * @brief Return the extension of a file by looking for the character '.'
 * @param filePath
 * @return return a pointer to the address after the '.' or null if it was not found;
 */
const char * file_extractExtension(const char * filePath);

/**
 * @brief Return the file name by looking for the last character '/'
 * @param filePath
 * @return return a pointer to the address after the last '/' or null if it was not found (the path might not be a path in this case)
 */
const char * file_extractFileName(const char * filePath);


GList * file_listFolderContent(const char * path);
#endif
