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

	error_t err = order_set_load_order(GAMES_APPIDS[current_game], plugins);
	return err == ERR_FAILURE;
}

static void on_plugin_up(GtkWidget *, gpointer user_data) {
	GList * list_node = user_data;
	GList * next_node = g_list_previous(list_node);

	if(next_node == NULL)
		return;

	gpointer * tmp = list_node->data;
	list_node->data = next_node->data;
	next_node->data = tmp;
	order_set_load_order(GAMES_APPIDS[current_game], plugins);
	plugin_tab_generate_ui();
}

static void on_plugin_down(GtkWidget *, gpointer user_data) {
	GList * list_node = user_data;
	GList * prev_node = g_list_next(list_node);

	if(prev_node == NULL)
		return;

	gpointer * tmp = list_node->data;
	list_node->data = prev_node->data;
	prev_node->data = tmp;
	order_set_load_order(GAMES_APPIDS[current_game], plugins);
	plugin_tab_generate_ui();
}



void plugin_tab_generate_ui(void) {
	if(plugin_tab_widget == NULL) {
		plugin_tab_widget = adw_bin_new();
	}
	GtkWidget * body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(body, 12);
	gtk_widget_set_margin_end(body, 12);
	adw_bin_set_child(ADW_BIN(plugin_tab_widget), body);

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
	GList * prev_it = NULL;
	while(plugins_iterator != NULL) {
		GList * next_it = g_list_next(plugins_iterator);
		order_plugin_entry_t * plugin = (order_plugin_entry_t *)plugins_iterator->data;
		GtkWidget * plugin_row = adw_action_row_new();
		adw_preferences_row_set_title(ADW_PREFERENCES_ROW(plugin_row), plugin->filename);
		adw_action_row_set_title_lines(ADW_ACTION_ROW(plugin_row),2);
		gtk_list_box_append(GTK_LIST_BOX(plugin_box), plugin_row);

		GtkWidget * button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
		adw_action_row_add_suffix(ADW_ACTION_ROW(plugin_row), button_box);

		GtkWidget * enable_switch = gtk_switch_new();
		gtk_switch_set_active(GTK_SWITCH(enable_switch), plugin->activated);
		gtk_switch_set_state(GTK_SWITCH(enable_switch), plugin->activated);
		gtk_widget_set_valign(enable_switch, GTK_ALIGN_CENTER);
		g_signal_connect(enable_switch, "state-set", G_CALLBACK(on_mod_toggled), plugin);
		gtk_box_append(GTK_BOX(button_box), enable_switch);

		GtkWidget * up_dow_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_add_css_class(up_dow_button_box, "linked");
		gtk_box_prepend(GTK_BOX(button_box), up_dow_button_box);

		GtkWidget * up_button = gtk_button_new();
		gtk_button_set_icon_name(GTK_BUTTON(up_button), "go-up-symbolic");
		gtk_box_append(GTK_BOX(up_dow_button_box), up_button);
		gtk_widget_set_valign(up_button, GTK_ALIGN_CENTER);
		g_signal_connect(up_button, "clicked", G_CALLBACK(on_plugin_up), plugins_iterator);
		if(prev_it == NULL) {
			gtk_widget_set_sensitive (up_button, FALSE);
		}

		GtkWidget * down_button = gtk_button_new();
		gtk_button_set_icon_name(GTK_BUTTON(down_button), "go-down-symbolic");
		gtk_box_append(GTK_BOX(up_dow_button_box), down_button);
		gtk_widget_set_valign(down_button, GTK_ALIGN_CENTER);
		g_signal_connect(down_button, "clicked", G_CALLBACK(on_plugin_down), plugins_iterator);
		if(next_it == NULL) {
			gtk_widget_set_sensitive (down_button, FALSE);
		}

		prev_it = plugins_iterator;
		plugins_iterator = next_it;
	}
	//g_list_free_full(plugins, free);

	return;
}
