#ifndef __ARCHIVES_H__
#define __ARCHIVES_H__

//all of these function will make every file into lowercase

/**
 * @brief Execute the unzip command
 * @param path path to archive
 * @param outdir output director
 * @return int return code
 */
int archive_unzip(char * path, char * outdir);

/**
 * @brief Execute the unrar command
 * @param path path to archive
 * @param outdir output director
 * @return int return code
 */
int archive_unrar(char * path, char * outdir);

/**
 * @brief Execute the 7z command
 * @param path path to archive
 * @param outdir output director
 * @return int return code
 */
int archive_un7z(char * path, const char * outdir);

#endif
