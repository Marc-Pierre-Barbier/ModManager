
extern "C" {
	#include <constants.h>
	
	#include <adwaita.h>
	#include <stdio.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <mods.h>
	#include <steam.h>
	#include <gtk/gtk.h>
	
	#include "gio/gio.h"
	#include "mod_tab.h"
	#include "file.h"
	#include "game_tab.h"
	#include "fomod_install.h"
}

#include <algorithm>
#include <ranges>
#include <filesystem>
#include <vector>

#include <case_adapted_path.hpp>

#include <fomodTypes.hpp>
#include <fomod.hpp>

static void on_next();

namespace fs = std::filesystem;

GtkWidget * popover_image;
GtkLabel * popover_description;


// fomod specific globs
std::vector<FOModFlag> flagList;
std::vector<FOModFile> pendingFileOperations;
std::vector<GtkWidget *> buttons;
AdwWindow * dialog_window;

bool install_in_progress = FALSE;
std::shared_ptr<FOMod> current_fomod;
unsigned int fomod_step_id;
unsigned int current_group_id;
unsigned int mod_id;
fs::path mod_folder;

//used for most one
GtkCheckButton * last_selected;
//used for least_one
int selection_count;

static void popover_on_enter(GtkEventControllerMotion*, gdouble, gdouble, gpointer user_data) {
	FOModPlugin * plugin = reinterpret_cast<FOModPlugin*>(user_data);
	//i have my doubts with the c_str
	gtk_image_set_from_file(GTK_IMAGE(popover_image), plugin->image.c_str());
	gtk_label_set_text(popover_description, plugin->description.c_str());
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

static std::vector<GtkWidget *> popover_checked(FOModGroup &group) {
	std::vector<GtkWidget *> buttons;
	last_selected = NULL;
	GtkWidget * first_button = NULL;
	selection_count = 1;
	for(auto &plugin : group.plugins) {
		GtkWidget * button = gtk_check_button_new();
		buttons.push_back(button);

		if(first_button == NULL)
				first_button = button;

		switch(group.type) {
		case GroupType::ONE_ONLY:
			if(first_button == button)
				gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
			else
				gtk_check_button_set_group(GTK_CHECK_BUTTON(button), GTK_CHECK_BUTTON(first_button));
			break;
		case GroupType::AT_LEAST_ONE:
			if(first_button == button)
				gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
			g_signal_connect(button, "toggled", G_CALLBACK(on_button_toggeled_least_one), NULL);
			break;
		case GroupType::ALL:
			gtk_widget_set_sensitive(button, FALSE);
			gtk_check_button_set_active(GTK_CHECK_BUTTON(button), TRUE);
			break;
		case GroupType::AT_MOST_ONE:
			g_signal_connect(button, "toggled", G_CALLBACK(on_button_toggeled_most_one), NULL);
			break;
		case GroupType::ANY:
			break;
		default:
			g_warning("unknown group");
			break;
		}

		GtkEventController * controller = gtk_event_controller_motion_new();
		g_signal_connect(controller, "enter", G_CALLBACK(popover_on_enter), reinterpret_cast<gpointer>(&plugin));

		gtk_widget_add_controller(button, controller);
		gtk_check_button_set_label(GTK_CHECK_BUTTON(button), plugin.name.c_str());
	}

	return buttons;
}

static void popover_fomod_container(FOModGroup &group) {
	//TODO: handle force closure
	dialog_window = ADW_WINDOW(adw_window_new());
	gtk_window_set_title(GTK_WINDOW(dialog_window), group.name.c_str());

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
	for(auto &button : buttons) {
		gtk_box_append(GTK_BOX(left_box), button);
	}
	popover_on_enter(NULL, 0, 0, reinterpret_cast<void *>(&group.plugins[0]));

	adw_window_set_content(dialog_window, root);
	gtk_window_present(GTK_WINDOW (dialog_window));
}

static void next_install_step() {
	FOModStep &step = current_fomod->steps[fomod_step_id];

	//checking if we collected all required flags for this step
	bool validFlags = true;
	for(auto &flag : step.required_flags) {
		auto flagLink = std::find_if(flagList.begin(), flagList.end(), [flag](const FOModFlag &item) {
			return item.name == flag.name && item.value == flag.value;
		});
		if(flagLink != flagList.end()) {
			validFlags = false;
			break;
		}
	}

	if(!validFlags) {
		return on_next();
	}

	popover_fomod_container(step.groups[current_group_id]);
}

static void last_install_step() {
	//triggerd when there is no next step

	//TODO: manage multiple files with the same name
	fomod_process_cond_files(*current_fomod, flagList, pendingFileOperations);
	
	pendingFileOperations.reserve(current_fomod->required_install_files.size());
	for(auto &file : current_fomod->required_install_files) {
		pendingFileOperations.push_back(file);
	}

	fomod_execute_file_operations(pendingFileOperations, mod_id, GAMES_APPIDS[current_game]);

	printf("FOMod successfully installed!\n");

	//cleanup
	pendingFileOperations = {};
	flagList = {};
	mod_folder = "";
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
	FOModStep &step = current_fomod->steps[fomod_step_id];
	FOModGroup &group = step.groups[current_group_id];

	std::vector<unsigned int> choices;
	unsigned int index = 0;
	for(GtkWidget * &widget : buttons) {
		GtkCheckButton * button = GTK_CHECK_BUTTON(widget);
		if(gtk_check_button_get_active(button)) {
			choices.push_back(index);
		}
		index++;
	}

	//process them
	for(auto choice : choices) {
		FOModPlugin &plugin = group.plugins[choice];
		flagList.reserve(plugin.flags.size());
		for(auto &flag : plugin.flags) {
			flagList.push_back(flag);
		}

		//do the install
		for(auto &file : plugin.files) {
			//copy the file implies that we need to also clone the strings in it
			pendingFileOperations.push_back(file);
		}
	}

	current_group_id += 1;
	if(step.groups.size() == current_group_id) {
		fomod_step_id += 1;
		current_group_id = 0;
	}

	if(current_fomod->steps.size() <= fomod_step_id) {
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

	mod_folder = fs::path(mods_folder) / std::string((char *)mod->data);
	std::string destination = std::string((char *)mod->data) + "__FOMOD";
	g_autofree GFile * destination_file = g_file_new_build_filename(mods_folder, destination.c_str(), NULL);

	if(g_file_query_exists(destination_file, NULL)) {
		if(!file_delete_recursive(destination_file, NULL, NULL)) {
			return ERR_FAILURE;
		}
	}

	
	auto details = mods_mod_details(appid, modid);
	fs::path relative_fomod_file = fs::path(details.fomod_file);
	g_list_free_full(mod_list, g_free);
	if(!fs::exists(relative_fomod_file)) {
		g_warning( "FOMod file not found, are you sure this is a fomod mod ?\n");
		return ERR_FAILURE;
	}

	fomod_step_id = 0;
	auto fomod = fomod_parse(relative_fomod_file);
	if(!fomod)
		return ERR_FAILURE;

	current_fomod = *fomod;

	next_install_step();
	return ERR_SUCCESS;
}
