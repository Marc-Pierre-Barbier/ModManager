#include <gtk/gtk.h>
#include <steam.h>
#include <adwaita.h>
#include <deploy.h>
#include "game_tab.h"
#include "mod_tab.h"
#include "plugin_tab.h"
#include "window.h"


int current_game = -1;
static int appid = 0;
static GHashTable * gamePaths;


static void on_undeploy();

static void on_deploy() {
	if(deploy(appid) == OK) {
		gtk_button_set_icon_name(start_button, "media-playback-stop-symbolic");
		g_signal_handlers_disconnect_by_func(start_button, on_deploy, NULL);
		g_signal_connect(start_button, "clicked", G_CALLBACK(on_undeploy), NULL);
	} else {
		printf("Failed to deploy\n");
	}
}

static void on_undeploy() {
	if(undeploy(appid) == ERR_SUCCESS) {
		gtk_button_set_icon_name(start_button, "media-playback-start-symbolic");
		g_signal_handlers_disconnect_by_func(start_button, on_undeploy, NULL);
		g_signal_connect(start_button, "clicked", G_CALLBACK(on_deploy), NULL);
	}
}

static void row_selected (GtkListBox*, GtkListBoxRow* row, gpointer) {
	if(row == NULL) {
		current_game = -1;
		return;
	}

	GList * gamesIds = g_hash_table_get_keys(gamePaths);
	int index = gtk_list_box_row_get_index(row);
	current_game = *(int *)g_list_nth_data(gamesIds, index);
	appid = GAMES_APPIDS[current_game];

	//enable the button
	bool status;
	error_t err = is_deployed(appid, &status);
	if(err != ERR_SUCCESS) {
		g_error("No button");
		//TODO: error handling
		gtk_button_set_icon_name(start_button, "edit-delete-symbolic");
		gtk_widget_set_sensitive(GTK_WIDGET(start_button), FALSE);
	} else {
		if(status) {
			g_signal_handlers_disconnect_by_func(start_button, on_deploy, NULL);
			g_signal_connect(start_button, "clicked", G_CALLBACK(on_undeploy), NULL);
			gtk_button_set_icon_name(start_button, "media-playback-stop-symbolic");
		} else {
			g_signal_handlers_disconnect_by_func(start_button, on_undeploy, NULL);
			g_signal_connect(start_button, "clicked", G_CALLBACK(on_deploy), NULL);
			gtk_button_set_icon_name(start_button, "media-playback-start-symbolic");
		}
		gtk_widget_set_sensitive(GTK_WIDGET(start_button), TRUE);
	}

	mod_tab_generate_ui();
	plugin_tab_generate_ui();
	g_list_free(gamesIds);
}

GtkWidget * game_tab_generate_ui(GtkWindow * window) {
	GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	error_t status = steam_search_games(&gamePaths);
	int table_size = g_hash_table_size(gamePaths);
	if(status == ERR_FAILURE) {
		GtkAlertDialog * alert = gtk_alert_dialog_new("Could not find your game library");
		gtk_alert_dialog_show(alert, window);
		gtk_window_present(window);
		return NULL;
	} else if(table_size == 0) {
		GtkAlertDialog * alert = gtk_alert_dialog_new("Could not find any compatible game");
		gtk_alert_dialog_show(alert, window);
		gtk_window_present(window);
		return NULL;
	}

	GtkWidget *title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<big>Pick the game you want to mod</big>");
	gtk_label_set_width_chars(GTK_LABEL(title), 80);
	gtk_box_append(GTK_BOX(body), title);

	GtkWidget * list_box = gtk_list_box_new();
	gtk_box_append(GTK_BOX(body), list_box);
	gtk_widget_add_css_class(list_box, "boxed-list");
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
	gtk_widget_set_margin_start(list_box, 12);
	gtk_widget_set_margin_end(list_box, 12);
	g_signal_connect(list_box, "row-selected", G_CALLBACK(row_selected), NULL);

	GList * gamesIds = g_hash_table_get_keys(gamePaths);
	GList * gameIdsIterator = gamesIds;
	while(gameIdsIterator != NULL) {
		const int gameIndex = *(int*)gameIdsIterator->data;

		GtkWidget * row = adw_action_row_new();
		adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), GAMES_NAMES[gameIndex]);

		gtk_list_box_append(GTK_LIST_BOX(list_box), row);
		gameIdsIterator = g_list_next(gameIdsIterator);
	}

	g_list_free(gamesIds);
	//TODO: memory deallocation
	//g_hash_table_destroy(gamePaths);

	return body;
}
