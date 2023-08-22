#include <gtk/gtk.h>
#include <adwaita.h>
#include <steam.h>
#include "game_tab.h"
#include "plugin_tab.h"
#include <loadOrder.h>

GtkWidget * plugin_tab_widget = NULL;
GList * plugins = NULL;

static gboolean on_mod_toggled(GtkSwitch*, gboolean state, gpointer user_data) {
	order_plugin_entry_t * plugin = (order_plugin_entry_t *)user_data;
	plugin->activated = state;

	error_t err = order_setLoadOrder(GAMES_APPIDS[current_game], plugins);
	return err == ERR_FAILURE;
}


void plugin_tab_generate_ui(void) {
	if(plugin_tab_widget == NULL) {
		plugin_tab_widget = gtk_viewport_new(NULL, NULL);
	}
	GtkWidget * body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(body, 12);
	gtk_widget_set_margin_end(body, 12);
	gtk_viewport_set_child(GTK_VIEWPORT(plugin_tab_widget), body);

	if(current_game == -1) {
		GtkWidget * status_page = adw_status_page_new();
		adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "No game selected");
		adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "dialog-error-symbolic");
		gtk_box_set_baseline_position(GTK_BOX(body), GTK_BASELINE_POSITION_CENTER);
		gtk_box_append(GTK_BOX(body), status_page);
		return;
	}

	error_t err = order_getLoadOrder(GAMES_APPIDS[current_game], &plugins);

	if(err != ERR_SUCCESS) {
		return;
	}

	if(plugins == NULL) {
		GtkWidget * status_page = adw_status_page_new();
		adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "No load order found. you need to start your game at least once");
		adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "help-about-symbolic");
		gtk_box_set_baseline_position(GTK_BOX(body), GTK_BASELINE_POSITION_CENTER);
		gtk_box_append(GTK_BOX(body), status_page);
		return;
	}

	GtkWidget * title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<big>Plugins</big>");
	gtk_box_prepend(GTK_BOX(body), title);

	GtkWidget * plugin_scrollable = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(plugin_scrollable, TRUE);
	gtk_box_append(GTK_BOX(body), plugin_scrollable);


	GtkWidget * plugin_box_wrap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(plugin_box_wrap, 12);
	gtk_widget_set_margin_end(plugin_box_wrap, 12);
	gtk_widget_set_margin_bottom(plugin_box_wrap, 12);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(plugin_scrollable), plugin_box_wrap);

	GtkWidget * plugin_box = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(plugin_box), GTK_SELECTION_NONE);
	gtk_widget_add_css_class(plugin_box, "boxed-list");
	gtk_box_append(GTK_BOX(plugin_box_wrap), plugin_box);

	GList * plugins_iterator = plugins;
	while(plugins_iterator != NULL) {
		order_plugin_entry_t * plugin = (order_plugin_entry_t *)plugins_iterator->data;
		GtkWidget * plugin_row = adw_action_row_new();
		adw_preferences_row_set_title(ADW_PREFERENCES_ROW(plugin_row), plugin->filename);
		GtkWidget * enable_switch = gtk_switch_new();
		gtk_switch_set_active(GTK_SWITCH(enable_switch), plugin->activated);
		gtk_switch_set_state(GTK_SWITCH(enable_switch), plugin->activated);
		g_signal_connect(enable_switch, "state-set", G_CALLBACK(on_mod_toggled), plugin);

		gtk_widget_set_valign(enable_switch, GTK_ALIGN_CENTER);
		adw_action_row_add_suffix(ADW_ACTION_ROW(plugin_row), enable_switch);
		adw_action_row_set_title_lines(ADW_ACTION_ROW(plugin_row),2);
		gtk_list_box_append(GTK_LIST_BOX(plugin_box), plugin_row);
		plugins_iterator = g_list_next(plugins_iterator);
	}
	//g_list_free_full(plugins, free);

	return;
}
