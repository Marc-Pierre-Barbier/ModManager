#include <fomod.h>
#include <constants.h>
#include <stdbool.h>
#include <adwaita.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mods.h>
#include <steam.h>
#include <gtk/gtk.h>

#include "mod_tab.h"
#include "file.h"
#include "game_tab.h"
#include "fomod_install.h"

GtkWidget * popover_image;
GtkLabel * popover_description;


// fomod specific globs
GList * flagList = NULL;
GList * pendingFileOperations = NULL;
GList * buttons;
AdwWindow * dialog_window;

bool install_in_progress = FALSE;
Fomod_t current_fomod;
int fomod_step_id;
int current_group_id;
int mod_id;
char * mod_folder;

//used for most one
GtkCheckButton * last_selected;
//used for least_one
int selection_count;

static void on_next();


static void popover_on_enter(GtkEventControllerMotion*, gdouble, gdouble, gpointer user_data) {
	FomodPlugin_t * plugin = user_data;
	char * imagepath = g_build_filename(mod_folder, plugin->image, NULL);
	gtk_image_set_from_file(GTK_IMAGE(popover_image), imagepath);
	gtk_label_set_text(popover_description, plugin->description);
}

static void popover_on_leave(GtkEventControllerMotion*, gdouble, gdouble, gpointer) {
	gtk_image_clear(GTK_IMAGE(popover_image));
	gtk_label_set_text(popover_description, NULL);
}

static void on_button_toggeled_least_one(GtkCheckButton* self, gpointer) {
	if(gtk_check_button_get_active(self)) {
		selection_count++;
	} else {
		if(selection_count == 1) {
			gtk_check_button_set_active(self, TRUE);
		} else {
			selection_count++;
		}
	}
}

static void on_button_toggeled_most_one(GtkCheckButton* self, gpointer) {
	/*
		gtk_check_button_set_active will trigger this event,
		keep that in mind while editing.
	*/
	if(last_selected != NULL) {
		gtk_check_button_set_active(last_selected, FALSE);
		if(last_selected == self) {
			last_selected = NULL;
			return;
		}
	}
	last_selected = self;
}

static GList * popover_checked(const FomodGroup_t *group) {
	last_selected = NULL;
	GList * buttons = NULL;
	GtkWidget * first_button = NULL;
	selection_count = 1;
	for(int i = 0; i < group->plugin_count; i++) {
		GtkWidget * button = gtk_check_button_new();
		buttons = g_list_append(buttons, button);

		if(first_button == NULL)
				first_button = button;

		switch(group->type) {
		case ONE_ONLY:
			if(first_button == button)
				gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
			else
				gtk_check_button_set_group(GTK_CHECK_BUTTON(button), GTK_CHECK_BUTTON(first_button));
			break;
		case AT_LEAST_ONE:
			if(first_button == button)
				gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
			g_signal_connect(button, "toggled", G_CALLBACK(on_button_toggeled_least_one), NULL);
			break;
		case ALL:
			gtk_widget_set_sensitive(button, FALSE);
			gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
			break;
		case AT_MOST_ONE:
			g_signal_connect(button, "toggled", G_CALLBACK(on_button_toggeled_most_one), NULL);
			break;
		case ANY:
			break;
		}

		GtkEventController * controller = gtk_event_controller_motion_new();
		g_signal_connect(controller, "enter", G_CALLBACK(popover_on_enter), &(group->plugins[i]));
		g_signal_connect(controller, "leave", G_CALLBACK(popover_on_leave), NULL);

		gtk_widget_add_controller(button, controller);
		gtk_check_button_set_label(GTK_CHECK_BUTTON(button), group->plugins[i].name);
	}

	return buttons;
}

static void popover_fomod_container(const FomodGroup_t *group) {
	//TODO: handle force closure
	dialog_window = ADW_WINDOW(adw_window_new());
	gtk_window_set_title(GTK_WINDOW(dialog_window), group->name);

	GtkWidget * root = adw_toolbar_view_new();
	GtkWidget *header = adw_header_bar_new();
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(root), header);
	const int WIDTH = 800;

	GtkWidget * left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_size_request(left_box, WIDTH*0.2, -1);
	GtkWidget * right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_size_request(right_box, WIDTH*0.8, -1);
	gtk_widget_set_hexpand(right_box, FALSE);

	GtkWidget * paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_resize_start_child (GTK_PANED (paned), TRUE);
	gtk_paned_set_shrink_start_child (GTK_PANED (paned), FALSE);
	gtk_paned_set_resize_end_child (GTK_PANED (paned), FALSE);
	gtk_paned_set_shrink_end_child (GTK_PANED (paned), FALSE);

	gtk_paned_set_resize_start_child (GTK_PANED (paned), FALSE);
	gtk_paned_set_shrink_start_child (GTK_PANED (paned), FALSE);



	gtk_paned_set_start_child(GTK_PANED(paned), left_box);
	gtk_paned_set_end_child(GTK_PANED(paned), right_box);
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(root), paned);

	GtkWidget * next_button = gtk_button_new();
	gtk_button_set_label(GTK_BUTTON(next_button), "Next");
	gtk_widget_set_valign(next_button, GTK_ALIGN_END);
	g_signal_connect(next_button, "clicked", G_CALLBACK(on_next), NULL);
	adw_toolbar_view_add_bottom_bar(ADW_TOOLBAR_VIEW(root), next_button);

	popover_image = gtk_image_new();
	gtk_image_set_pixel_size(GTK_IMAGE(popover_image), WIDTH*0.5);
	gtk_box_append(GTK_BOX(right_box), popover_image);
	popover_description = GTK_LABEL(gtk_label_new(NULL));
	gtk_label_set_wrap(popover_description, TRUE);
	gtk_label_set_justify(popover_description, GTK_JUSTIFY_CENTER);
	gtk_box_append(GTK_BOX(right_box), GTK_WIDGET(popover_description));

	buttons = popover_checked(group);
	GList * btn_iterator = buttons;
	while(btn_iterator != NULL) {
		gtk_box_append(GTK_BOX(left_box), btn_iterator->data);
		btn_iterator = g_list_next(btn_iterator);
	}
	popover_on_enter(NULL, 0, 0, &group->plugins[0]);

	adw_window_set_content(dialog_window, root);
	gtk_window_present(GTK_WINDOW (dialog_window));
}

//match the definirion of gcompare func
static gint fomod_flag_equal(const FomodFlag_t * a, const FomodFlag_t * b) {
	int nameCmp = strcmp(a->name, b->name);
	if(nameCmp == 0) {
		if(strcmp(a->value, b->value) == 0)
			//is equal
			return 0;

		return 1;
	}
	return nameCmp;
}

static void next_install_step() {
	const FomodStep_t * step = &current_fomod.steps[fomod_step_id];

	//checking if we collected all required flags for this step
	bool validFlags = true;
	for(int flagId = 0; flagId < step->flag_count; flagId++) {
		const GList * flagLink = g_list_find_custom(flagList, &step->required_flags[flagId], (GCompareFunc)fomod_flag_equal);
		if(flagLink == NULL) {
			validFlags = false;
			break;
		}
	}

	if(!validFlags) {
		fomod_step_id += 1;
		return;
	}

	//don't remember
	FomodGroup_t * group = &step->groups[current_group_id];
	popover_fomod_container(group);
}

static void last_install_step() {
	//triggerd when there is no next step

	//TODO: manage multiple files with the same name
	pendingFileOperations = fomod_process_cond_files(&current_fomod, flagList, pendingFileOperations);
	fomod_process_file_operations(&pendingFileOperations, mod_id, GAMES_APPIDS[current_game]);

	printf("FOMod successfully installed!\n");

	//cleanup
	fomod_free_fomod(&current_fomod);
	fomod_free_file_operations(pendingFileOperations);
	pendingFileOperations = NULL;
	g_list_free(flagList);
	flagList = NULL;
	g_free(mod_folder);
	mod_folder = NULL;
	install_in_progress = FALSE;
	last_selected = NULL;
	fomod_step_id = 0;
	current_group_id = 0;
	mod_id = 0;

	mod_tab_generate_ui();
}


static void on_next() {
	gtk_window_close(GTK_WINDOW(dialog_window));
	//extract user choices
	const FomodStep_t * step = &current_fomod.steps[fomod_step_id];
	FomodGroup_t * group = &step->groups[current_group_id];
	GList * choices = NULL;
	GList * btn_iterator = buttons;
	unsigned int index = 0;
	while(btn_iterator != NULL) {
		GtkCheckButton * button = GTK_CHECK_BUTTON(btn_iterator->data);
		if(gtk_check_button_get_active(button)) {
			unsigned int * id = (unsigned int *)g_malloc(sizeof(unsigned int));
			*id = index;
			choices = g_list_append(choices, id);
		}
		index++;
		btn_iterator = g_list_next(btn_iterator);
	}

	//process them
	GList * choice_iterator = choices;
	while(choice_iterator != NULL) {
		int choice = *(int *)choice_iterator->data;
		choice_iterator = g_list_next(choice_iterator);

		FomodPlugin_t * plugin = &group->plugins[choice];
		for(int flagId = 0; flagId < plugin->flag_count; flagId++) {
			flagList = g_list_append(flagList, &plugin->flags[flagId]);
		}

		//do the install
		for(int pluginId = 0; pluginId < plugin->file_count; pluginId++) {
			FomodFile_t * file = g_malloc(sizeof(FomodFile_t));
			//copy the file implies that we need to also clone the strings in it
			*file = plugin->files[pluginId];
			file->destination = strdup(file->destination);
			file->source = strdup(file->source);

			pendingFileOperations = g_list_append(pendingFileOperations, file);
		}
	}

	g_list_free_full(choices, g_free);

	current_group_id += 1;
	if(step->group_count == current_group_id) {
		fomod_step_id += 1;
		current_group_id = 1;
	}

	if(current_fomod.step_count == fomod_step_id) {
		last_install_step();
	} else {
		next_install_step();
	}
}



error_t gui_fomod_installer(int modid) {
	if(install_in_progress) {
		return ERR_FAILURE;
	}
	install_in_progress = TRUE;
	mod_id = modid;

	int appid = GAMES_APPIDS[current_game];
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	//might crash if no mods were installed
	const char * home_path = g_get_home_dir();
	g_autofree char * mods_folder = g_build_filename(home_path, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);
	GList * mod_list = mods_list(appid);
	GList * mod = g_list_nth(mod_list, mod_id);

	if(mod == NULL) {
		fprintf(stderr, "Mod not found\n");
		return ERR_FAILURE;
	}

	g_autofree char * destination = g_strconcat(mod->data, "__FOMOD", NULL);
	g_autofree GFile * destination_file = g_file_new_build_filename(mods_folder, destination, NULL);

	if(g_file_query_exists(destination_file, NULL)) {
		if(!file_delete_recursive(destination_file, NULL, NULL)) {
			return ERR_FAILURE;
		}
	}

	mod_folder = g_build_filename(mods_folder, mod->data, NULL);
	g_list_free_full(mod_list, g_free);

	//everything should be lowercase since we use casefold() before calling any install function
	g_autofree char * fomod_folder = g_build_filename(mod_folder, "fomod", NULL);
	g_autofree GFile * fomod_file = g_file_new_build_filename(fomod_folder, "moduleconfig.xml", NULL);

	if(!g_file_query_exists(fomod_file, NULL)) {
		g_warning( "FOMod file not found, are you sure this is a fomod mod ?\n");
		return ERR_FAILURE;
	}

	fomod_step_id = 0;
	int returnValue = fomod_parse(fomod_file, &current_fomod);
	if(returnValue == ERR_FAILURE)
		return ERR_FAILURE;

	next_install_step();
	return ERR_SUCCESS;
}
