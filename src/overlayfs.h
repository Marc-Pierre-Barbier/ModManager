#ifndef __OVERLAY_H__
#define __OVERLAY_H__

/**
 * @brief Overlayfs is what make it possible to deploy to the game files without altering anything.
 * it allows us to overlay multiple folder over the game files. like appling filters to an image.
 * @param sources a null ended table of folder that will overlay of the game files
 * @param dest the overlayed folder(the game data folder)
 * @param upperdir the director that will store the changed files
 * @param workdir a directory that will contains only temporary files.
 * @return int error code
 */
int overlayMount(char ** sources, const char * dest, const char * upperdir, const char * workdir);

#endif
