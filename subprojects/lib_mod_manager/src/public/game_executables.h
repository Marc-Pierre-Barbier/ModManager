#include <errorType.h>
#include <gio/gio.h>

[[nodiscard]]error_t list_game_executables(int appid, GList ** executables);
[[nodiscard]]error_t set_game_executable(int appid, const char * executable);
[[nodiscard]]char * get_game_executable(int appid);
[[nodiscard]]const char * get_default_game_executable(int appid);
