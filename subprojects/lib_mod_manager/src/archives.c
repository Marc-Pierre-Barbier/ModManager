#include "archives.h"
#include <file.h>

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <archive.h>
#include <archive_entry.h>

#define BLOCK_SIZE 16384

static int copy_data(struct archive *input, struct archive *output)
{
	int r;
	const void *buff;
	size_t size;
	la_int64_t offset;

	for (;;) {
		r = archive_read_data_block(input, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r < ARCHIVE_OK)
			return (r);
		r = archive_write_data_block(output, buff, size, offset);
		if (r < ARCHIVE_OK) {
			fprintf(stderr, "%s\n", archive_error_string(output));
			return (r);
		}
	}
}

archive_error_t archive_deflate(GFile * file, GFile * g_out_dir) {
	g_autofree char * path = g_file_get_path(file);
	g_autofree char * out_dir = g_file_get_path(g_out_dir);

	if(!g_file_test(out_dir, G_FILE_TEST_IS_DIR)) {
		g_mkdir_with_parents(out_dir, 0755);
	}

	//save cwd to restore it at the end of the function
	char initial_work_dir[PATH_MAX];
	getcwd(initial_work_dir, PATH_MAX); //TODO: handle error

	chdir(out_dir);

	struct archive *input;
	struct archive *output;
	struct archive_entry *entry;
	int flags;
	int r;

	/* Select which attributes we want to restore. */
	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;

	input = archive_read_new();
	archive_read_support_format_all(input);
	archive_read_support_filter_all(input);
	output = archive_write_disk_new();
	archive_write_disk_set_options(output, flags);
	archive_write_disk_set_standard_lookup(output);

	if ((r = archive_read_open_filename(input, path, BLOCK_SIZE)))
		return AR_ERR_NOT_FOUND;

	for (;;) {
		r = archive_read_next_header(input, &entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(input));
		if (r < ARCHIVE_WARN)
			exit(1);

		r = archive_write_header(output, entry);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(output));
		else if (archive_entry_size(entry) > 0) {
			r = copy_data(input, output);
			if (r < ARCHIVE_OK)
				fprintf(stderr, "%s\n", archive_error_string(output));
			if (r < ARCHIVE_WARN)
				exit(1);
		}

		r = archive_write_finish_entry(output);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(output));
		if (r < ARCHIVE_WARN)
			exit(1);
	}

	archive_read_close(input);
	archive_read_free(input);
	archive_write_close(output);
	archive_write_free(output);

	file_casefold(g_out_dir);
	chdir(initial_work_dir);
	return AR_ERR_OK;
}
