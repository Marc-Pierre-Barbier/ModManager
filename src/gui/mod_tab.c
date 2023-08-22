#include <glib.h>
#include <steam.h>
#include <mods.h>
#include <adwaita.h>
#include "mod_tab.h"
#include "game_tab.h"

GtkWidget * mod_tab_widget = NULL;
GList * mods;

static gboolean on_mod_toggled(GtkSwitch*, gboolean state, gpointer user_data) {
	int modId = g_list_index(mods, user_data);

	error_t err;
	if(state)
		err = mods_enable_mod(GAMES_APPIDS[current_game], modId);
	else
		err = mods_disable_mod(GAMES_APPIDS[current_game], modId);

	return err == ERR_FAILURE;
}

void mod_tab_generate_ui() {
	if(mod_tab_widget == NULL) {
		mod_tab_widget = gtk_viewport_new(NULL, NULL);
	}
	GtkWidget * body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(body, 12);
	gtk_widget_set_margin_end(body, 12);
	gtk_viewport_set_child(GTK_VIEWPORT(mod_tab_widget), body);

	if(current_game == -1) {
		GtkWidget * status_page = adw_status_page_new();
		adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "No game selected");
		adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "dialog-error-symbolic");
		gtk_box_set_baseline_position(GTK_BOX(body), GTK_BASELINE_POSITION_CENTER);
		gtk_box_append(GTK_BOX(body), status_page);
		return;
	}


	GtkWidget * title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<big>Mods</big>");
	gtk_box_prepend(GTK_BOX(body), title);

	GtkWidget * mod_scrollable = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(mod_scrollable, TRUE);
	gtk_box_append(GTK_BOX(body), mod_scrollable);

	GtkWidget * mod_box_wrap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(mod_box_wrap, 12);
	gtk_widget_set_margin_end(mod_box_wrap, 12);
	gtk_widget_set_margin_bottom(mod_box_wrap, 12);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(mod_scrollable), mod_box_wrap);

	GtkWidget * mod_box = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(mod_box), GTK_SELECTION_NONE);
	gtk_widget_add_css_class(mod_box, "boxed-list");
	gtk_widget_set_margin_bottom(mod_box, 12);
	gtk_box_append(GTK_BOX(mod_box_wrap), mod_box);

	mods = mods_list(GAMES_APPIDS[current_game]);
	GList * mods_iterator = mods;
	int index = 0;
	while(mods_iterator != NULL) {
		const char * mod_name = (char *)mods_iterator->data;
		mods_mod_detail_t details = mods_mod_details(GAMES_APPIDS[current_game], index);
		if(!details.has_fomod_sibling) {
			GtkWidget * mod_row = adw_action_row_new();

			adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mod_row), mod_name);
			adw_action_row_set_title_lines(ADW_ACTION_ROW(mod_row),2);

			if(details.is_fomod)
				adw_action_row_set_subtitle(ADW_ACTION_ROW(mod_row), "This is a fomod mod");

			GtkWidget * enable_switch = gtk_switch_new();
			gtk_widget_set_valign(enable_switch, GTK_ALIGN_CENTER);
			adw_action_row_add_suffix(ADW_ACTION_ROW(mod_row), enable_switch);
			gtk_switch_set_active(GTK_SWITCH(enable_switch), details.is_activated);
			gtk_switch_set_state(GTK_SWITCH(enable_switch), details.is_activated);
			g_signal_connect(enable_switch, "state-set", G_CALLBACK(on_mod_toggled), mods_iterator->data);

			gtk_list_box_append(GTK_LIST_BOX(mod_box), mod_row);
		} else {
			printf("ignoring mod %s since it's has a fomod sibling\n", mod_name);
		}
		index++;
		mods_iterator = g_list_next(mods_iterator);
	}
}
