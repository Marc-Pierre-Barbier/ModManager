#include <fomod.h>

/**
 * @brief Start a terminal based install process for fomod.
 *
 * @param modFolder folder of the fomod file.
 * @param destination folder of the new mod that contains the result of the fomod process.
 * @return int
 */
error_t fomod_installFOMod(GFile * modFolder, GFile * destination);
