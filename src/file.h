#include <stdbool.h>
#include <sys/types.h>

#ifndef __FILE_H__
#define __FILE_H__


//valid copy flags
#define cp_DEFAULT 0
#define CP_LINK 1
#define CP_RECURSIVE 2
#define CP_NO_TARGET_DIR 4

/**
 * @brief execute the cp command from the source to the dest
 * @param source path to the source files
 * @param dest path to the destination folder
 * @param flags refer to the "valid copy flags" use them like this CP_LINK | CP_RECURSIVE
 * @return status code
 */
int copy(const char * source, const char * dest, u_int32_t flags);

/**
 * @brief execute the cp command from the source to the dest
 * @param source path to the source files
 * @param dest path to the destination folder
*  @param bool enable recursive rm -r
 * @return status code
 */
int delete(const char * path, bool recursive);

/**
 * @brief Run the mv command
 * @param source
 * @param destination
 * @return mv's exit value
 */
int move(const char * source, const char * destination);

/**
 * @brief Recursively rename all file and folder to lowercase.
 *
 * @param folder
 */
void casefold(const char * folder);

#endif
