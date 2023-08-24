#ifndef __ARCHIVES_H__
#define __ARCHIVES_H__

#include <gio/gio.h>

//all of these function will make every file into lowercase

/**
 * @brief Execute the unzip command
 * @param path path to archive
 * @param outdir output director
 * @return int return code
 */
int archive_unzip(GFile * file, GFile * outdir);

/**
 * @brief Execute the unrar command
 * @param path path to archive
 * @param outdir output director
 * @return int return code
 */
int archive_unrar(GFile * file, GFile * out_dir);

/**
 * @brief Execute the 7z command
 * @param path path to archive
 * @param outdir output director
 * @return int return code
 */
int archive_un7z(GFile * file, GFile * outdir);

#endif
