#include <adwaita.h>
#include "game_tab.h"
#include "plugin_tab.h"
#include "mod_tab.h"


static GtkApplication *app;
GtkWidget *window;

static void game_selector(void) {
	window = adw_application_window_new(app);

	gtk_window_set_title(GTK_WINDOW(window),  "Mod manager - Pick your game");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	// Create a new window
	GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	GtkWidget *header = adw_header_bar_new();
	gtk_box_append(GTK_BOX(root), header);

	GtkWidget * viewswitcher = adw_view_switcher_title_new();

	GtkWidget *content = adw_view_stack_new();
	gtk_box_append(GTK_BOX(root), content);

	GtkWidget * game_tab_widget = game_tab_generate_ui(GTK_WINDOW(window));
	mod_tab_generate_ui();
	plugin_tab_generate_ui();

	AdwViewStackPage* game_page = adw_view_stack_add(ADW_VIEW_STACK(content), game_tab_widget);
	adw_view_stack_page_set_title(game_page, "Game");
	adw_view_stack_page_set_icon_name(game_page, "applications-games-symbolic");

	AdwViewStackPage* mod_page = adw_view_stack_add(ADW_VIEW_STACK(content), mod_tab_widget);
	adw_view_stack_page_set_title(mod_page, "Mods");
	adw_view_stack_page_set_icon_name(mod_page, "applications-engineering-symbolic");

	AdwViewStackPage* plugin_page = adw_view_stack_add(ADW_VIEW_STACK(content), plugin_tab_widget);
	adw_view_stack_page_set_title(plugin_page, "Plugins");
	adw_view_stack_page_set_icon_name(plugin_page, "preferences-other-symbolic");

	adw_view_switcher_title_set_stack(ADW_VIEW_SWITCHER_TITLE(viewswitcher), ADW_VIEW_STACK(content));

	adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), viewswitcher);

	adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), root);
	gtk_window_present(GTK_WINDOW(window));
}

int main (int argc, char *argv[]) {
	// Create a new application
	app = GTK_APPLICATION(adw_application_new ("fr.marcbarbier.modmanager", G_APPLICATION_DEFAULT_FLAGS));
	g_signal_connect (app, "activate", G_CALLBACK(game_selector), NULL);
	return g_application_run (G_APPLICATION(app), argc, argv);
}
