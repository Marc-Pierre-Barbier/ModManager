#include <libaudit.h>
#include <pwd.h>
#include <string.h>
#include "getHome.h"
#include <gio/gio.h>

GFile * audit_get_home(void) {
    //not getting home from the env enable us to use sudo
	struct passwd *pw = getpwuid(audit_getloginuid());
	return g_file_new_for_path(pw->pw_dir);
}
