#ifndef __GAME_TAB_H__
#define __GAME_TAB_H__
#include <gtk/gtk.h>

extern int current_game; // -1 mean no game selected

GtkWidget * game_tab_generate_ui(GtkWindow * window);


#endif
