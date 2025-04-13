// fs.h - In-Memory Filesystem
#ifndef FS_H
#define FS_H

#include "common.h"

// File types
#define FS_FILE 0
#define FS_DIRECTORY 1

// Maximum filename length
#define FS_MAX_FILENAME 64
#define FS_MAX_PATH 256

// Maximum number of directory entries
#define FS_MAX_DIR_ENTRIES 64

// Forward declarations
struct fs_node;

// Function pointer types
typedef uint32_t (*read_type_t)(struct fs_node *, uint32_t, uint32_t, uint8_t *);
typedef uint32_t (*write_type_t)(struct fs_node *, uint32_t, uint32_t, uint8_t *);
typedef void (*open_type_t)(struct fs_node *);
typedef void (*close_type_t)(struct fs_node *);
typedef struct fs_node *(*finddir_type_t)(struct fs_node *, char *);
typedef struct fs_node *(*mkdir_type_t)(struct fs_node *, char *);
typedef struct fs_node *(*create_type_t)(struct fs_node *, char *, uint8_t);
typedef int (*unlink_type_t)(struct fs_node *, char *);

// Filesystem node structure
typedef struct fs_node
{
    char name[FS_MAX_FILENAME];
    uint8_t type;      // File or directory?
    uint32_t size;     // Size in bytes
    uint32_t created;  // Creation timestamp
    uint32_t modified; // Last modification timestamp

    // Function pointers for filesystem operations
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    finddir_type_t finddir;
    mkdir_type_t mkdir;
    create_type_t create;
    unlink_type_t unlink;

    // For directories, list of children
    struct fs_node **children;
    uint32_t child_count;

    // For files, actual data
    uint8_t *data;

    // Parent node (for .. navigation)
    struct fs_node *parent;
} fs_node_t;

// Global root node
extern fs_node_t *fs_root;

// Filesystem API
void fs_init();
uint32_t fs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t fs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void fs_open(fs_node_t *node);
void fs_close(fs_node_t *node);
fs_node_t *fs_finddir(fs_node_t *node, char *name);
fs_node_t *fs_mkdir(fs_node_t *parent, char *name);
fs_node_t *fs_create(fs_node_t *parent, char *name, uint8_t type);
int fs_unlink(fs_node_t *parent, char *name);

// Path manipulation functions
fs_node_t *fs_resolve_path(const char *path);
char *fs_basename(const char *path);
char *fs_dirname(const char *path);

#endif // FS_H