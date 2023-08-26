#include <fomod.h>
#include <constants.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mods.h>
#include <steam.h>
#include <getHome.h>
#include <gtk/gtk.h>

#include "file.h"
#include "game_tab.h"
#include "fomod_install.h"

pthread_mutex_t popover_mutex;
GtkWidget * popover_image;
GtkWidget * popover_description;

//used for most one
GtkCheckButton * last_selected;
//used for least_one
int selection_count;

static void on_popover_close(GtkPopover*, gpointer) {
	pthread_mutex_unlock(&popover_mutex);
}

static void popover_on_enter(GtkEventControllerMotion*, gdouble, gdouble, gpointer user_data) {
	FomodPlugin_t * plugin = user_data;
	gtk_image_set_from_file(GTK_IMAGE(popover_image), plugin->image);
	gtk_label_set_text(GTK_LABEL(popover_description), plugin->description);
}

static void popover_on_leave(GtkEventControllerMotion*, gdouble, gdouble, gpointer) {
	gtk_image_clear(GTK_IMAGE(popover_image));
	gtk_label_set_text(GTK_LABEL(popover_description), "");
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
	if(last_selected != NULL) {
		if(last_selected == self) {
			gtk_check_button_set_active(last_selected, TRUE);
			return;
		}
		gtk_check_button_set_active(last_selected, FALSE);
	}
	last_selected = self;
}

static GList * popover_checked(const FomodGroup_t *group) {
	last_selected = NULL;
	GtkWidget * left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
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

		gtk_box_append(GTK_BOX(left_box), button);
		gtk_widget_add_controller(button, controller);
		gtk_button_set_label(GTK_BUTTON(button), group->plugins[i].name);
	}

	return buttons;
}

static GList * popover_fomod_container(const FomodGroup_t *group, GList * (*left_pan_gen)(const FomodGroup_t *group)) {
	GtkWidget * popover = gtk_popover_new();
	GtkWidget * left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	GtkWidget * right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	GtkWidget * paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_start_child(GTK_PANED(paned), left_box);
	gtk_paned_set_end_child(GTK_PANED(paned), right_box);
	gtk_popover_set_child(GTK_POPOVER(popover), paned);

	GList * buttons = left_pan_gen(group);
	GList * btn_iterator = buttons;
	while(btn_iterator != NULL) {
		gtk_box_append(GTK_BOX(left_box), btn_iterator->data);
		btn_iterator = g_list_next(btn_iterator);
	}


	popover_image = gtk_image_new();
	gtk_box_append(GTK_BOX(right_box), popover_image);
	popover_description = gtk_label_new(NULL);
	gtk_box_append(GTK_BOX(right_box), popover_description);

	gtk_popover_popup(GTK_POPOVER(popover));
	g_signal_connect(popover, "closed", G_CALLBACK(on_popover_close), NULL);
	pthread_mutex_lock(&popover_mutex);

	GList * selected_indicies = NULL;
	btn_iterator = buttons;
	unsigned int index = 0;
	while(btn_iterator != NULL) {
		GtkCheckButton * button = GTK_CHECK_BUTTON(btn_iterator->data);
		if(gtk_check_button_get_active(button)) {
			unsigned int * id = (unsigned int *)g_malloc(sizeof(unsigned int));
			*id = index;
			selected_indicies = g_list_append(selected_indicies, id);
		}
		index++;
		btn_iterator = g_list_next(btn_iterator);
	}

	return selected_indicies;
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

error_t gui_fomod_installer(int modid) {
	pthread_mutex_init(&popover_mutex, NULL);
	int appid = GAMES_APPIDS[current_game];
	char appid_str[9];
	snprintf(appid_str, 9, "%d", appid);

	//might crash if no mods were installed
	g_autofree GFile * home = audit_get_home();
	g_autofree char * home_path = g_file_get_path(home);
	g_autofree char * mods_folder = g_build_filename(home_path, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);
	GList * mod_list = mods_list(appid);
	GList * mod = g_list_nth(mod_list, modid);

	if(mod == NULL) {
		fprintf(stderr, "Mod not found\n");
		return ERR_FAILURE;
	}

	g_autofree char * destination = g_strconcat(mod->data, "__FOMOD", NULL);
	g_autofree GFile * destination_file = g_file_new_build_filename(destination, NULL);

	if(g_file_query_exists(destination_file, NULL)) {
		if(!file_delete_recursive(destination_file, NULL, NULL)) {
			return ERR_FAILURE;
		}
	}
	if(!g_file_make_directory_with_parents(destination_file, NULL, NULL)) {
		g_error( "Could not create output folder\n");
		return ERR_FAILURE;
	}

	g_autofree char * mod_folder = g_build_filename(mod_folder, mod->data, NULL);
	g_list_free_full(mod_list, g_free);

	//everything should be lowercase since we use casefold() before calling any install function
	g_autofree char * fomod_folder = g_build_filename(mods_folder, "fomod", NULL);
	g_autofree GFile * fomod_file = g_file_new_build_filename(fomod_folder, "moduleconfig.xml", NULL);

	if(!g_file_query_exists(fomod_file, NULL)) {
		g_error( "FOMod file not found, are you sure this is a fomod mod ?\n");
		return ERR_FAILURE;
	}

	Fomod_t fomod;
	int returnValue = fomod_parse(fomod_file, &fomod);
	if(returnValue == ERR_FAILURE)
		return ERR_FAILURE;


	GList * flagList = NULL;
	GList * pendingFileOperations = NULL;

	for(int i = 0; i < fomod.step_count; i++) {
		const FomodStep_t * step = &fomod.steps[i];

		//checking if we collected all required flags for this step
		bool validFlags = true;
		for(int flagId = 0; flagId < step->flag_count; flagId++) {
			const GList * flagLink = g_list_find_custom(flagList, &step->required_flags[flagId], (GCompareFunc)fomod_flag_equal);
			if(flagLink == NULL) {
				validFlags = false;
				break;
			}
		}
		if(!validFlags) continue;

		//don't remember
		for(int groupId = 0; groupId < step->group_count; groupId++ ) {
			FomodGroup_t * group = &step->groups[groupId];

			GList * choices = popover_fomod_container(group, popover_checked);
			if(choices == NULL)
				return ERR_FAILURE;

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
					FomodFile_t * file = &plugin->files[pluginId];
					//changing pathes to lowercase since we used casefold and the pathes in the xml might not like it
					file->destination = g_ascii_strdown(file->destination, -1);

					char * source = g_ascii_strdown(file->source, -1);
					file->source = strdup(source);
					g_free(source);

					pendingFileOperations = g_list_append(pendingFileOperations, file);
				}
			}

			g_list_free_full(choices, g_free);
		}
	}

	pthread_mutex_destroy(&popover_mutex);

	//TODO: manage multiple files with the same name

	pendingFileOperations = fomod_process_cond_files(&fomod, flagList, pendingFileOperations);
	fomod_process_file_operations(&pendingFileOperations, modid, GAMES_APPIDS[current_game]);

	printf("FOMod successfully installed!\n");
	g_list_free(flagList);
	fomod_free_file_operations(pendingFileOperations);
	fomod_free_fomod(&fomod);
	return ERR_SUCCESS;
}
