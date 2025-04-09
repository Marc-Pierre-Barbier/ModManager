#include "settings_tab.h"
#include <game_executables.h>
#include <adwaita.h>
#include <string.h>
#include "errorType.h"
#include "game_tab.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtk.h"
#include "gtk/gtkdropdown.h"
#include "gtk/gtkshortcut.h"
#include "steam.h"

/*

Transform this in to a dropdown swith a launch button wich allows user to run executables like fnis without goind through steam.

Maybe also rethink the "launch" button to also launch steam steam://rungameid/APPID
hook detection should not be useful as it can skip deployment.

*/
GtkWidget * settings_tab_widget;

static void executable_dropdown_handler(GtkWidget * dropdown, gpointer user_data) {
	const int index = gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown));
	GList * executables = NULL;
	error_t err = list_game_executables(GAMES_APPIDS[current_game], &executables);
	if(err == ERR_FAILURE) {
		g_error("Failed to fetch executables\n");
		return;
	}

	const char * current_executable = g_list_nth(executables, index)->data;
	err = set_game_executable(GAMES_APPIDS[current_game], current_executable);
	if(err == ERR_FAILURE)
		g_error("Failed to set executables\n");

	g_list_free_full(executables, free);

}

void settings_tab_generate_ui() {
	if(settings_tab_widget == NULL) {
		settings_tab_widget = adw_bin_new();
	}

	GtkWidget * body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(body, 12);
	gtk_widget_set_margin_end(body, 12);
	adw_bin_set_child(ADW_BIN(settings_tab_widget), body);

	if(current_game == -1) {
		GtkWidget * status_page = adw_status_page_new();
		adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "No game selected");
		adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "dialog-error-symbolic");
		gtk_box_set_baseline_position(GTK_BOX(body), GTK_BASELINE_POSITION_CENTER);
		gtk_box_append(GTK_BOX(body), status_page);
		return;
	}

	const int appid = GAMES_APPIDS[current_game];
	GList * executables = NULL;
	error_t err = list_game_executables(appid, &executables);
	g_autofree const char * current_executable = get_game_executable(appid);

	if(err != ERR_SUCCESS) {
		GtkWidget * status_page = adw_status_page_new();
		adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Failed to load executables");
		adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "help-about-symbolic");
		gtk_box_set_baseline_position(GTK_BOX(body), GTK_BASELINE_POSITION_CENTER);
		gtk_box_append(GTK_BOX(body), status_page);
		return;
	}

	GtkWidget * title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<big>Settings</big>");
	gtk_box_prepend(GTK_BOX(body), title);

	GtkWidget * settingss_scrollable = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(settingss_scrollable, TRUE);
	gtk_box_append(GTK_BOX(body), settingss_scrollable);


	GtkWidget * settingss_box_wrap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(settingss_box_wrap, 12);
	gtk_widget_set_margin_end(settingss_box_wrap, 12);
	gtk_widget_set_margin_bottom(settingss_box_wrap, 12);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(settingss_scrollable), settingss_box_wrap);

	const int executables_length = g_list_length(executables);
	const char * drop_down_list[executables_length + 1];

	GList * executables_iterator = executables;
	int current_executable_index = 0;
	for(int i = 0; i < executables_length; i++) {
		g_autofree const gchar * filename = g_path_get_basename(executables_iterator->data);
		drop_down_list[i] = strdup(executables_iterator->data);
		if(strcmp(current_executable, filename) == 0)
			current_executable_index = i;

		executables_iterator = g_list_next(executables_iterator);
	}
	drop_down_list[executables_length] = NULL;

	GtkWidget * executables_dropdown_label = gtk_label_new("Default executable:");
	GtkWidget * executables_dropdown = gtk_drop_down_new_from_strings(drop_down_list);
	gtk_drop_down_set_selected(GTK_DROP_DOWN(executables_dropdown), current_executable_index);
	gtk_box_append(GTK_BOX(settingss_box_wrap), executables_dropdown_label);
	gtk_box_append(GTK_BOX(settingss_box_wrap), executables_dropdown);
	g_signal_connect_after(executables_dropdown, "notify::selected", G_CALLBACK(executable_dropdown_handler), NULL);

	/*GtkWidget * settingss_box = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(settingss_box), GTK_SELECTION_NONE);
	gtk_widget_add_css_class(settingss_box, "boxed-list");
	gtk_box_append(GTK_BOX(settingss_box_wrap), settingss_box);

	for(GList * settingss_iterator = settingss; settingss_iterator != NULL; settingss_iterator = g_list_next(settingss_iterator)) {
		const char * settings_file = settingss_iterator->data;
		g_autofree const char * filename = g_path_get_basename(settings_file);

		GtkWidget * plugin_row = adw_action_row_new();
		adw_preferences_row_set_title(ADW_PREFERENCES_ROW(plugin_row), filename);
		adw_action_row_set_title_lines(ADW_ACTION_ROW(plugin_row),2);
		gtk_list_box_append(GTK_LIST_BOX(settingss_box), plugin_row);

		GtkWidget * button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
		adw_action_row_add_suffix(ADW_ACTION_ROW(plugin_row), button_box);

		GtkWidget * enable_switch = gtk_switch_new();
		gboolean is_current = strcmp(current_settings, filename) == 0;

		gtk_switch_set_active(GTK_SWITCH(enable_switch), is_current);
		gtk_switch_set_state(GTK_SWITCH(enable_switch), is_current);
		gtk_widget_set_sensitive (enable_switch, !is_current);

		gtk_widget_set_valign(enable_switch, GTK_ALIGN_CENTER);
		//g_signal_connect(enable_switch, "state-set", G_CALLBACK(on_mod_toggled), plugin);
		gtk_box_append(GTK_BOX(button_box), enable_switch);
	}*/

	g_list_free_full(executables, free);
}
