#include <optional>
#include <vector>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 18)
#define autofree __attribute__((cleanup(cleanup_free)))

#include <fuse3/fuse.h>
#include <filesystem>
#include <algorithm>
#include <unordered_set>


namespace fs = std::filesystem;

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include <libgen.h>

char * mountpoint;

//call free for the cleanup attribute
static void cleanup_free(void *p) {
	void * pointer = *(void**) p;
	if(pointer != NULL)
		free(pointer);
}

static struct options {
	const char *upperdir;
	const char *lowerdirs;
	const char *contents;
	int show_help;
} options;

#define OPTION(t, p)                           \
{ t, offsetof(struct options, p), 1 }

static const struct fuse_opt option_spec[] = {
	OPTION("--upperdir=%s", upperdir),
	OPTION("--lowerdirs=%s", lowerdirs),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

static int modfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);

static void usage(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system required options:\n"
		   "    --upperdir=<s>          Output folder for new files\n"
		   "    --lowerdirs=<s>          Comma sperated list of directories to fallback to\n"
		   "\n");
}

static void *modfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
	//no caching we are using the underlying fs'
	if (!cfg->auto_cache) {
		cfg->entry_timeout = 0;
		cfg->attr_timeout = 0;
		cfg->negative_timeout = 0;
	}

	return NULL;
}

static int modfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int modfs_open(const char *path, struct fuse_file_info *fi);
static int modfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int modfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int modfs_mkdir(const char *path, mode_t mode);
static int modfs_unlink(const char *path);
static int modfs_rmdir(const char *path);
static int modfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

static const struct fuse_operations mod_operations = {
	.getattr = modfs_getattr,
	.mkdir = modfs_mkdir,
	.unlink = modfs_unlink,
	.rmdir = modfs_rmdir,
	.open = modfs_open,
	.read = modfs_read,
	.write = modfs_write,
	.readdir = modfs_readdir,
	.init = modfs_init,
	.create = modfs_create,
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	options.lowerdirs = NULL;
	options.upperdir = NULL;

	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	if(options.lowerdirs == NULL || options.upperdir == NULL) {
		printf("Missing lowerdirs and/or upperdir options\n");
		options.show_help = 1;
	}

	if (options.show_help) {
		usage(argv[0]);
		//make fuse show the help message
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		//remove the first line from fuse's help
		args.argv[0][0] = '\0';
	}

	mountpoint = args.argv[args.argc - 1];

	int ret = fuse_main(args.argc, args.argv, &mod_operations, NULL);
	fuse_opt_free_args(&args);
	return ret;
}

template<class F>
static inline void dirent_iterator(fs::path path, F callback) {
	DIR * dp = opendir(path.c_str());
	if (dp == NULL)
		return;

	struct dirent * de;
	while ((de = readdir(dp)) != NULL) {
		callback(dp, de);
	}
	closedir(dp);
}

static std::optional<fs::path> case_adapted_path(fs::path fspath, fs::path refrence_dir) {
	std::vector<std::string> path;

	fs::path badpath = fspath;
	while(badpath.has_parent_path()) { //fail in precense of tailing sperators
		auto name = badpath.filename();
		if(!name.empty()) //handle tailing sperators
			path.push_back(name);
		badpath = badpath.parent_path();
	}

	while(!path.empty()) {
		std::string filename = *path.rbegin();
		transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
		path.pop_back();

		bool found = false;
		for(auto entry : fs::directory_iterator(refrence_dir)) {
			const auto on_disk_name = entry.path().filename().string();
			std::string on_disk_name_lower = on_disk_name;
			transform(on_disk_name_lower.begin(), on_disk_name_lower.end(), on_disk_name_lower.begin(), ::tolower);

			if(filename == on_disk_name_lower) {
				refrence_dir /= on_disk_name;
				found = true;
				break;
			}
		}

		if(!found) {
			return std::nullopt;
		}
	}

	return refrence_dir;
}

static int modfs_readdir(fs::path path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
	std::stringstream ss(options.lowerdirs);
	std::unordered_set<std::string> entries{};
	std::string lowerdir;

	auto process_path = [&](fs::path target) {
		dirent_iterator(target, [&](DIR * dp, struct dirent * de){
			struct stat st;
			std::string name = de->d_name;
			std::string lower_name = name;
			transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

			if(entries.contains(lower_name)) {
				//already found need to hide
				return;
			}

			fstatat(dirfd(dp), de->d_name, &st,	AT_SYMLINK_NOFOLLOW);
			filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS);
			entries.emplace(lower_name);
		});
	};

	auto adapted_path = case_adapted_path(path, options.upperdir);
	if(adapted_path)
		process_path(*adapted_path);

	while (getline(ss, lowerdir, ',')) {
		auto adapted_path = case_adapted_path(path, lowerdir);
		if(adapted_path)
			process_path(*adapted_path);
	}

	return 0;
}
static int modfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
	return modfs_readdir(fs::path("." + std::string(path)), buf, filler, offset, fi, flags);
}

static std::optional<fs::path> search(fs::path path) {
	auto guessed_path = case_adapted_path(path, options.upperdir);
	if(guessed_path) {
		return guessed_path;
	}

	std::stringstream ss(options.lowerdirs);
	std::string lowerdir;
	while (getline(ss, lowerdir, ',')) {
		auto guessed_path = case_adapted_path(path, lowerdir);
		if(guessed_path) {
			return guessed_path;
		}
	}
	return std::nullopt;
}

static int modfs_getattr(fs::path path, struct stat *stbuf, struct fuse_file_info *fi) {
	auto found_path = search(path);

	if(!found_path && path == "./") {
		found_path = options.upperdir;
	}

	if(!found_path)
		return -ENOENT;

	int res = lstat(found_path->c_str(), stbuf);
	if (res == -1)
		return -errno;

	return 0;
}
static int modfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
	return modfs_getattr("." + std::string(path), stbuf, fi);
}

static int modfs_rmdir(fs::path path) {
	int res;
	std::optional<fs::path> target;
	
	//the least performant filesystem on the planet O(n^2) unlink
	while((target = search(path))) {
		res = rmdir(target->c_str());
		if (res == -1)
			return -errno;
	}
	return res;
}
static int modfs_rmdir(const char *path) {
	return modfs_rmdir("." + std::string(path));
}

static int modfs_unlink(fs::path path) {
	int res;
	std::optional<fs::path> target;

	//the least performant filesystem on the planet O(n^2) unlink
	while((target = search(path))) {
		res = unlink(target->c_str());
		if (res == -1)
			return -errno;
	}
	return res;
}
static int modfs_unlink(const char *path) {
	return modfs_unlink("." + std::string(path));
}

//for use in the upper dir only
static int recurse_mkdir(fs::path fspath, fs::path reference_path, mode_t mode) {
	int ret = 0;

	auto path = case_adapted_path(fspath.parent_path(), reference_path);
	if(!path) //if there is no parent make it
		ret = recurse_mkdir(fspath.parent_path(), reference_path, mode);

	if(ret != 0)
		return ret;
	//make the thing
	return mkdir((*path / fspath.filename()).c_str(), mode);
}

static int modfs_mkdir(fs::path path, mode_t mode) {
	std::optional<fs::path> found_path = search(path);
	if(found_path) {
		//path already exists let mkdir handle it and manages errors
		return mkdir(found_path->c_str(), mode);
	}

	std::optional<fs::path> found_parent_dir = search(path.parent_path());
	if(!found_parent_dir) {
		return -ENOENT;
	}

	auto newpath = (*found_parent_dir) /  path.filename();

	//absolutely awful to make
	return recurse_mkdir(path, options.upperdir, mode);
}
static int modfs_mkdir(const char *path, mode_t mode) {
	return modfs_mkdir("." + std::string(path), mode);
}

static int modfs_write(fs::path path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	int fd = 0;
	if(fi == NULL) {
		autofree auto file = search(path);
		if(!file)
			return -ENOENT;

		fd = open(file->c_str(), O_WRONLY);
	} else {
		fd = fi->fh;
	}

	if(fd == -1)
		return -errno;

	int res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);

	return res;
}
static int modfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	return modfs_write("." + std::string(path), buf, size, offset, fi);
}

static int modfs_read(fs::path path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	int fd = 0;
	if(fi == NULL) {
		autofree auto file = search(path);
		if(!file)
			return -ENOENT;

		fd = open(file->c_str(), O_RDONLY);
	} else {
		fd = fi->fh;
	}

	if(fd == -1)
		return -errno;

	int res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);

	return res;
}
static int modfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	return modfs_read("." + std::string(path), buf, size, offset, fi);
}

static int modfs_open(fs::path path, struct fuse_file_info *fi){
	auto searched_path = search(path);
	//doesn't exists
	if(!searched_path) {
		searched_path = case_adapted_path(path.parent_path(), options.upperdir);
		if(!searched_path) //parent dir doesn't exists => can't open
			return -ENOENT;

		searched_path = (*searched_path) / path.filename();
	}

	//create it in the upperdir
	int res = open(searched_path->c_str(), O_RDWR);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}
static int modfs_open(const char *path, struct fuse_file_info *fi){
	return modfs_open("." + std::string(path), fi);
}


static int modfs_create(fs::path path, mode_t mode, struct fuse_file_info *fi) {
	printf("Modfs create\n");
	auto searched_path = search(path);
	//already exists
	if(searched_path) {
		return 0;
	}

	//create in upperdir
	searched_path = case_adapted_path(path.parent_path(), options.upperdir);
	if(!searched_path) //parent dir doesn't exists => can't create
		return -ENOENT;

	fs::path full_path = (*searched_path) / path.filename();
	int res = open(full_path.c_str(), fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}
static int modfs_create(const char * path, mode_t mode, struct fuse_file_info *fi) {
	return modfs_create("." + std::string(path), mode, fi);
}