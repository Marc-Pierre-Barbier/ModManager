#ifndef __FILE_H__
#define __FILE_H__

#include <glib.h>
#include <sys/types.h>

#include "errorType.h"

#include <gio/gio.h>

/**
 * @brief Return the extension of a file by looking for the character '.'
 * @param filePath
 * @return return a pointer to the address after the '.' or null if it was not found;
 */
const char * file_extract_extension(const char * filePath);


gboolean file_delete_recursive(GFile *file, GCancellable *cancellable, GError **error);
error_t file_recursive_copy(GFile * src, GFile * dest, GFileCopyFlags flags, GCancellable *cancellable, GError **error);

#endif
