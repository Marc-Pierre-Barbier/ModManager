#include <adwaita.h>
#include "game_tab.h"
#include "plugin_tab.h"
#include "mod_tab.h"
#include <constants.h>


static GtkApplication *app;
GtkWidget *window;


static void show_about(GSimpleAction*, GVariant*, gpointer)
{
	const char *developers[] = {
		"Marc barbier <marc@marcbarbier.fr>",
		NULL
	};

	adw_show_about_window (
		gtk_application_get_active_window (app),
		"application-name", "Mod Manager",
		"application-icon", "insert-object-symbolic",
		"version", "GUI 0.0.1 - LIB " MODLIB_VERSION ,
		"copyright", "© 2023 Marc barbier",
		"issue-url", "https://gitlab.gnome.org/example/example/-/issues/new",
		"license-type", GTK_LICENSE_GPL_2_0,
		"developers", developers,
		NULL
	);
}


static void game_selector(void) {
	window = adw_application_window_new(app);
	gtk_window_set_icon_name(GTK_WINDOW(window), "insert-object-symbolic");
	#ifdef DEBUG
	gtk_widget_add_css_class(window, "devel");
	#endif

	gtk_window_set_title(GTK_WINDOW(window),	"Mod manager - Pick your game");
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


	GtkWidget * title_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), title_widget);
	adw_view_switcher_title_set_stack(ADW_VIEW_SWITCHER_TITLE(viewswitcher), ADW_VIEW_STACK(content));
	gtk_box_append(GTK_BOX(title_widget), viewswitcher);
	gtk_widget_set_hexpand(viewswitcher, TRUE);
	gtk_widget_set_hexpand_set(viewswitcher, TRUE);

	GtkWidget * burger_menu = gtk_menu_button_new();
	gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(burger_menu), "open-menu-symbolic");
	gtk_box_append(GTK_BOX(title_widget), burger_menu);


	GMenu * menu = g_menu_new();
	GSimpleAction *action_about = g_simple_action_new("about", NULL);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action_about));
    g_signal_connect(action_about, "activate", G_CALLBACK(show_about), NULL);

	GMenuItem * about_menu_item = g_menu_item_new("About", "app.about");
	g_menu_append_item(menu, about_menu_item);
	GtkWidget * popover_menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
	gtk_popover_set_autohide(GTK_POPOVER(popover_menu), TRUE);
	gtk_popover_popdown(GTK_POPOVER(popover_menu));
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(burger_menu), popover_menu);

	adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), root);
	gtk_window_present(GTK_WINDOW(window));
}

int main (int argc, char *argv[]) {
	// Create a new application
	app = GTK_APPLICATION(adw_application_new ("fr.marcbarbier.modmanager", G_APPLICATION_DEFAULT_FLAGS));
	g_signal_connect (app, "activate", G_CALLBACK(game_selector), NULL);
	return g_application_run (G_APPLICATION(app), argc, argv);
}
