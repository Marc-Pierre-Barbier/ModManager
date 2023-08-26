#include <glib.h>
#include <steam.h>
#include <mods.h>
#include <adwaita.h>
#include "fomod_install.h"
#include "mod_tab.h"
#include "game_tab.h"
#include "window.h"

GtkWidget * mod_tab_widget = NULL;
static GList * mods = NULL;
static GtkListBox * mod_box = NULL;

static gboolean on_mod_toggled(GtkSwitch*, gboolean state, gpointer user_data) {
	const int mod_id = g_list_index(mods, user_data);

	error_t err;
	if(state)
		err = mods_enable_mod(GAMES_APPIDS[current_game], mod_id);
	else
		err = mods_disable_mod(GAMES_APPIDS[current_game], mod_id);

	return err == ERR_FAILURE;
}

static void on_fomod_install(GtkMenuButton *, gpointer user_data) {
	const int mod_id = g_list_index(mods, user_data);
	error_t err = gui_fomod_installer(mod_id);
	if(err != ERR_SUCCESS) {
		//TODO: error popup
		printf("err %d\n", mod_id);
	} else {
		mod_tab_generate_ui();
	}
}

static void on_mod_deleted(GtkMenuButton *, gpointer user_data) {
	const int mod_id = g_list_index(mods, user_data);
	error_t err = mods_remove_mod(GAMES_APPIDS[current_game], mod_id);
	if(err != ERR_SUCCESS) {
		//TODO: error popup
		printf("err %d\n", mod_id);
	} else {
		mod_tab_generate_ui();
	}
}

static void on_mod_up(GtkWidget *, gpointer user_data) {
	const int mod_id = g_list_index(mods, user_data);
	error_t err = mods_swap_place(GAMES_APPIDS[current_game], mod_id, mod_id - 1);
		if(err != ERR_SUCCESS) {
		//TODO: error popup
		printf("err %d\n", mod_id);
	} else {
		mod_tab_generate_ui();
	}
}

static void on_mod_down(GtkWidget *, gpointer user_data) {
	const int mod_id = g_list_index(mods, user_data);
	error_t err = mods_swap_place(GAMES_APPIDS[current_game], mod_id, mod_id + 1);
		if(err != ERR_SUCCESS) {
		//TODO: error popup
		printf("err %d\n", mod_id);
	} else {
		mod_tab_generate_ui();
	}
}

static GtkWidget * create_mod_row(int mod_id, char * mod_name, bool first, bool last) {
	mods_mod_detail_t details = mods_mod_details(GAMES_APPIDS[current_game], mod_id);

	if(details.has_fomod_sibling)
		return NULL;

	GtkWidget * mod_row = adw_action_row_new();

	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mod_row), mod_name);
	adw_action_row_set_title_lines(ADW_ACTION_ROW(mod_row),2);

	if(details.is_fomod)
		adw_action_row_set_subtitle(ADW_ACTION_ROW(mod_row), "This is a fomod mod");

	GtkWidget * enable_switch = gtk_switch_new();
	gtk_widget_set_valign(enable_switch, GTK_ALIGN_CENTER);
	gtk_switch_set_active(GTK_SWITCH(enable_switch), details.is_activated);
	gtk_switch_set_state(GTK_SWITCH(enable_switch), details.is_activated);
	g_signal_connect(enable_switch, "state-set", G_CALLBACK(on_mod_toggled), mod_name);

	GtkWidget * remove_button = gtk_button_new();
	gtk_button_set_icon_name(GTK_BUTTON(remove_button), "user-trash-symbolic");
	gtk_widget_set_valign(remove_button, GTK_ALIGN_CENTER);
	g_signal_connect(remove_button, "clicked", G_CALLBACK(on_mod_deleted), mod_name);

	GtkWidget * button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(button_box), remove_button);
	gtk_box_append(GTK_BOX(button_box), enable_switch);

	GtkWidget * up_dow_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(up_dow_button_box, "linked");

	GtkWidget * up_button = gtk_button_new();
	gtk_button_set_icon_name(GTK_BUTTON(up_button), "go-up-symbolic");
	gtk_box_append(GTK_BOX(up_dow_button_box), up_button);
	gtk_widget_set_valign(up_button, GTK_ALIGN_CENTER);
	g_signal_connect(up_button, "clicked", G_CALLBACK(on_mod_up), mod_name);
	if(first) {
		gtk_widget_set_sensitive (up_button, FALSE);
	}

	GtkWidget * down_button = gtk_button_new();
	gtk_button_set_icon_name(GTK_BUTTON(down_button), "go-down-symbolic");
	gtk_box_append(GTK_BOX(up_dow_button_box), down_button);
	gtk_widget_set_valign(down_button, GTK_ALIGN_CENTER);
	g_signal_connect(down_button, "clicked", G_CALLBACK(on_mod_down), mod_name);
	if(last) {
		gtk_widget_set_sensitive (down_button, FALSE);
	}

	gtk_box_prepend(GTK_BOX(button_box), up_dow_button_box);

	if(details.has_fomodfile) {
		GtkWidget * fomod_button = gtk_button_new();
		gtk_button_set_icon_name(GTK_BUTTON(fomod_button), "document-properties-symbolic");
		gtk_box_prepend(GTK_BOX(button_box), fomod_button);
		gtk_widget_set_valign(fomod_button, GTK_ALIGN_CENTER);
		g_signal_connect(fomod_button, "clicked", G_CALLBACK(on_fomod_install), mod_name);
	}



	adw_action_row_add_suffix(ADW_ACTION_ROW(mod_row), button_box);
	return mod_row;
}

static void on_file_chooser_open_add_mod(GObject* source_object, GAsyncResult* res, gpointer) {
	GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source_object), res, NULL);
	if(file == NULL)
		return;

	error_t err = mods_add_mod(file, GAMES_APPIDS[current_game]);
	if(err != ERR_SUCCESS) {
		//TODO: error popup
		printf("err cound not add file\n");
	} else {
		mod_tab_generate_ui();
	}
}

static void open_add_mod_file_choser() {
	GtkFileDialog * dialog = gtk_file_dialog_new();
	gtk_file_dialog_open(dialog, GTK_WINDOW(window), NULL, on_file_chooser_open_add_mod, NULL);

}

void mod_tab_generate_ui() {
	if(mod_tab_widget == NULL) {
		mod_tab_widget = adw_bin_new();
	}

	if(current_game == -1) {
		GtkWidget * status_page = adw_status_page_new();
		adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "No game selected");
		adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "dialog-error-symbolic");
		adw_bin_set_child(ADW_BIN(mod_tab_widget), status_page);
		return;
	}

	if(mods != NULL) {
		g_list_free_full(mods, free);
	}

	mods = mods_list(GAMES_APPIDS[current_game]);

	GtkWidget * body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(body, 12);
	gtk_widget_set_margin_end(body, 12);
	adw_bin_set_child(ADW_BIN(mod_tab_widget), body);

	GtkWidget * header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_prepend(GTK_BOX(body), header);

	GtkWidget * title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<big>Mods</big>");
	gtk_box_prepend(GTK_BOX(header), title);

	GtkWidget * add_mod_button = gtk_button_new();
	gtk_box_append(GTK_BOX(header), add_mod_button);
	gtk_button_set_icon_name(GTK_BUTTON(add_mod_button), "document-new-symbolic");
	g_signal_connect(add_mod_button, "clicked", G_CALLBACK(open_add_mod_file_choser), NULL);

	GtkWidget * mod_scrollable = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(mod_scrollable, TRUE);
	gtk_box_append(GTK_BOX(body), mod_scrollable);

	GtkWidget * mod_box_wrap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_margin_start(mod_box_wrap, 12);
	gtk_widget_set_margin_end(mod_box_wrap, 12);
	gtk_widget_set_margin_bottom(mod_box_wrap, 12);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(mod_scrollable), mod_box_wrap);

	mod_box = GTK_LIST_BOX(gtk_list_box_new());
	gtk_list_box_set_selection_mode(mod_box, GTK_SELECTION_NONE);
	gtk_widget_add_css_class(GTK_WIDGET(mod_box), "boxed-list");
	gtk_widget_set_margin_bottom(GTK_WIDGET(mod_box), 12);
	gtk_box_append(GTK_BOX(mod_box_wrap), GTK_WIDGET(mod_box));

	mods = mods_list(GAMES_APPIDS[current_game]);
	GList * mods_iterator = mods;
	int index = 0;
	GList * prev_it = NULL;
	while(mods_iterator != NULL) {
		GList * next_it = g_list_next(mods_iterator);
		char * mod_name = (char *)mods_iterator->data;

		GtkWidget * mod_row = create_mod_row(index, mod_name, prev_it == NULL, next_it == NULL);
		if(mod_row != NULL) {
			gtk_list_box_append(GTK_LIST_BOX(mod_box), mod_row);
		} else {
			printf("ignoring mod %s since it's has a fomod sibling\n", mod_name);
		}
		index++;
		prev_it = mods_iterator;
		mods_iterator = next_it;
	}
}
