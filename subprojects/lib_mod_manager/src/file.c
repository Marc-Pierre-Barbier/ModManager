#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>

#include "file.h"

//rename a folder and all subfolder and files to lowercase
//TODO: error handling
error_t file_casefold(GFile * folder) {
	g_autofree char * folder_path = g_file_get_path(folder);
	if (g_file_query_exists(folder, NULL)) {
		GFileType file_type = g_file_query_file_type(folder, G_FILE_QUERY_INFO_NONE, NULL);
		if(file_type != G_FILE_TYPE_DIRECTORY)
			return ERR_FAILURE;

		GFileEnumerator *enumerator = g_file_enumerate_children(folder, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		for (GFileInfo *info = g_file_enumerator_next_file(enumerator, NULL, NULL); info != NULL; info = g_file_enumerator_next_file(enumerator, NULL, NULL)) {
			const char * name = g_file_info_get_name(info);
			//only look at ascii and hope for the best.
			g_autofree GFile * file = g_file_new_build_filename(folder_path, name, NULL);

			g_autofree gchar * destination_name = g_ascii_strdown(name, -1);
			g_autofree GFile * destination = g_file_new_build_filename(folder_path, destination_name, NULL);

			if(strcmp(destination_name, name) != 0) {
				if(!g_file_move(file, destination, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL)) {
					g_warning( "Move failed: %s => %s \n", name, destination_name);
				}
			}
			GFileType type = g_file_query_file_type(destination, G_FILE_QUERY_INFO_NONE, NULL);

			if(type == G_FILE_TYPE_DIRECTORY) {
				file_casefold(destination);
			}

		}
		g_object_unref(enumerator);
	}
	return ERR_SUCCESS;
}

static const char * file_extract_last_part(const char * file_path, const char delimeter) {
	const int length = strlen(file_path);
	long index = -1;
	for(long i= length - 1; i >= 0; i--) {
		if(file_path[i] == delimeter) {
			index = i + 1;
			break;
		}
	}

	if(index <= 0 || index == length) return "";
	return &file_path[index];
}

const char * file_extract_extension(const char * file_path) {
	return file_extract_last_part(file_path, '.');
}


error_t file_recursive_copy(GFile * src, GFile * dest, GFileCopyFlags flags, GCancellable *cancellable, GError **error) {
	//TODO: add error handling
// stolen from https://stackoverflow.com/questions/16453739/how-do-i-recursively-copy-a-directory-using-vala
	GFileType src_type = g_file_query_file_type(src, G_FILE_QUERY_INFO_NONE, cancellable);
	if (src_type == G_FILE_TYPE_DIRECTORY) {
		g_file_make_directory(dest, cancellable, error);
		g_file_copy_attributes(src, dest, flags, cancellable, error);

		GFileEnumerator *enumerator = g_file_enumerate_children(src, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, cancellable, error);
		for (GFileInfo *info = g_file_enumerator_next_file(enumerator, cancellable, error); info != NULL; info = g_file_enumerator_next_file(enumerator, cancellable, error)) {
			const char *relative_path = g_file_info_get_name(info);
			file_recursive_copy(
				g_file_resolve_relative_path(src, relative_path),
				g_file_resolve_relative_path(dest, relative_path),
				flags, cancellable, error
			);
		}
		g_object_unref(enumerator);
	} else if (src_type == G_FILE_TYPE_REGULAR) {
		g_file_copy(src, dest, flags, cancellable, NULL, NULL, error);
	}
	return ERR_SUCCESS;
}

gboolean file_delete_recursive(GFile *file, GCancellable *cancellable, GError **error) {
// stolen from https://stackoverflow.com/questions/16453739/how-do-i-recursively-copy-a-directory-using-vala
	GFileType file_type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, cancellable);
	if (file_type == G_FILE_TYPE_DIRECTORY) {
		GFileEnumerator *enumerator = g_file_enumerate_children(file, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, cancellable, error);
		for (GFileInfo *info = g_file_enumerator_next_file(enumerator, cancellable, error); info != NULL; info = g_file_enumerator_next_file(enumerator, cancellable, error)) {
			file_delete_recursive(
				g_file_resolve_relative_path(file, g_file_info_get_name(info)),
				cancellable, error
			);
		}
		g_object_unref(enumerator);
		g_file_delete(file, cancellable, error);
	} else if (file_type == G_FILE_TYPE_REGULAR) {
		g_file_delete(file, cancellable, error);
	}

	return TRUE;
}
