#ifndef __ARCHIVES_H__
#define __ARCHIVES_H__

#include <gio/gio.h>

typedef enum archive_error
{
	AR_ERR_OK,
	AR_ERR_NOT_FOUND
} archive_error_t;

//all of these function will make every file into lowercase

/**
 * @brief decompress archive
 * @param path path to archive
 * @param outdir output director
 * @return //TODO:
 */
archive_error_t archive_deflate(GFile * file, GFile * outdir);

#endif
