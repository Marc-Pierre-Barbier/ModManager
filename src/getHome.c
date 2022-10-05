#include <libaudit.h>
#include <pwd.h>
#include <string.h>
#include "getHome.h"

char * getHome(void) {
    //not getting home from the env enable us to use sudo
	struct passwd *pw = getpwuid(audit_getloginuid());
	return strdup(pw->pw_dir);
}
