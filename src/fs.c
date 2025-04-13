// fs.c - In-Memory Filesystem implementation
#include "fs.h"
#include "kmalloc.h"
#include "fb.h"
#include "string.h"

// Global root node
fs_node_t *fs_root = NULL;

// Get the current time (placeholder - would use RTC if available)
static uint32_t get_current_time()
{
    // In a real OS, this would use the RTC
    static uint32_t fake_time = 0;
    return ++fake_time;
}

// Initialize the filesystem
void fs_init()
{
    // Create the root directory
    fs_root = (fs_node_t *)kmalloc(sizeof(fs_node_t));

    if (!fs_root)
    {
        fb_write_string("Failed to initialize filesystem\n", FB_RED, FB_BLACK);
        return;
    }

    // Initialize root directory
    memset(fs_root, 0, sizeof(fs_node_t));
    strcpy(fs_root->name, "/");
    fs_root->type = FS_DIRECTORY;
    fs_root->size = 0;
    fs_root->created = get_current_time();
    fs_root->modified = fs_root->created;

    // Set function pointers
    fs_root->read = NULL;  // Directories can't be read directly
    fs_root->write = NULL; // Directories can't be written directly
    fs_root->open = NULL;
    fs_root->close = NULL;
    fs_root->finddir = fs_finddir;
    fs_root->mkdir = fs_mkdir;
    fs_root->create = fs_create;
    fs_root->unlink = fs_unlink;

    // Allocate children array
    fs_root->children = (fs_node_t **)kmalloc(sizeof(fs_node_t *) * FS_MAX_DIR_ENTRIES);
    if (!fs_root->children)
    {
        fb_write_string("Failed to allocate directory entries\n", FB_RED, FB_BLACK);
        kfree(fs_root);
        fs_root = NULL;
        return;
    }

    // Clear children array
    memset(fs_root->children, 0, sizeof(fs_node_t *) * FS_MAX_DIR_ENTRIES);
    fs_root->child_count = 0;

    // Root is its own parent (special case)
    fs_root->parent = fs_root;

    fb_write_string("Filesystem initialized\n", FB_GREEN, FB_BLACK);
}

// Read from a file
uint32_t fs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    // Check if node exists and is a file
    if (!node || node->type != FS_FILE || !node->data)
    {
        return 0;
    }

    // Check if offset is valid
    if (offset >= node->size)
    {
        return 0;
    }

    // Adjust size if it would go past the end of the file
    if (offset + size > node->size)
    {
        size = node->size - offset;
    }

    // Copy data to buffer
    memcpy(buffer, node->data + offset, size);

    return size;
}

// Write to a file
uint32_t fs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    // Check if node exists and is a file
    if (!node || node->type != FS_FILE)
    {
        return 0;
    }

    // If file is empty or too small, allocate/reallocate data
    if (!node->data || offset + size > node->size)
    {
        uint32_t new_size = offset + size;
        uint8_t *new_data = (uint8_t *)kmalloc(new_size);

        if (!new_data)
        {
            return 0; // Out of memory
        }

        // Copy existing data if any
        if (node->data)
        {
            memcpy(new_data, node->data, node->size);
            // Zero out any gap between old size and offset
            if (offset > node->size)
            {
                memset(new_data + node->size, 0, offset - node->size);
            }
            kfree(node->data);
        }
        else
        {
            // Zero out everything up to offset
            memset(new_data, 0, offset);
        }

        node->data = new_data;
        node->size = new_size;
    }

    // Copy data from buffer
    memcpy(node->data + offset, buffer, size);

    // Update modification time
    node->modified = get_current_time();

    return size;
}

// Open a file (for now, just a placeholder)
void fs_open(fs_node_t *node)
{
    if (node && node->open)
    {
        node->open(node);
    }
}

// Close a file (for now, just a placeholder)
void fs_close(fs_node_t *node)
{
    if (node && node->close)
    {
        node->close(node);
    }
}

// Find a directory entry by name
fs_node_t *fs_finddir(fs_node_t *node, char *name)
{
    // Check if node is a directory
    if (!node || node->type != FS_DIRECTORY)
    {
        return NULL;
    }

    // Handle special case for "."
    if (strcmp(name, ".") == 0)
    {
        return node;
    }

    // Handle special case for ".."
    if (strcmp(name, "..") == 0)
    {
        return node->parent;
    }

    // Search for the entry
    for (uint32_t i = 0; i < node->child_count; i++)
    {
        if (strcmp(node->children[i]->name, name) == 0)
        {
            return node->children[i];
        }
    }

    return NULL; // Not found
}

// Create a directory
fs_node_t *fs_mkdir(fs_node_t *parent, char *name)
{
    // Check if parent is a directory and has space for a new entry
    if (!parent || parent->type != FS_DIRECTORY || parent->child_count >= FS_MAX_DIR_ENTRIES)
    {
        return NULL;
    }

    // Check if name already exists
    if (fs_finddir(parent, name))
    {
        return NULL; // Already exists
    }

    // Allocate new directory node
    fs_node_t *dir = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    if (!dir)
    {
        return NULL; // Out of memory
    }

    // Initialize directory
    memset(dir, 0, sizeof(fs_node_t));
    strcpy(dir->name, name);
    dir->type = FS_DIRECTORY;
    dir->size = 0;
    dir->created = get_current_time();
    dir->modified = dir->created;

    // Set function pointers
    dir->read = NULL;
    dir->write = NULL;
    dir->open = NULL;
    dir->close = NULL;
    dir->finddir = fs_finddir;
    dir->mkdir = fs_mkdir;
    dir->create = fs_create;
    dir->unlink = fs_unlink;

    // Allocate children array
    dir->children = (fs_node_t **)kmalloc(sizeof(fs_node_t *) * FS_MAX_DIR_ENTRIES);
    if (!dir->children)
    {
        kfree(dir);
        return NULL; // Out of memory
    }

    // Clear children array
    memset(dir->children, 0, sizeof(fs_node_t *) * FS_MAX_DIR_ENTRIES);
    dir->child_count = 0;

    // Set parent
    dir->parent = parent;

    // Add to parent's children
    parent->children[parent->child_count++] = dir;

    // Update parent modification time
    parent->modified = dir->created;

    return dir;
}

// Create a file
fs_node_t *fs_create(fs_node_t *parent, char *name, uint8_t type)
{
    // Check if parent is a directory and has space for a new entry
    if (!parent || parent->type != FS_DIRECTORY || parent->child_count >= FS_MAX_DIR_ENTRIES)
    {
        return NULL;
    }

    // Check if name already exists
    if (fs_finddir(parent, name))
    {
        return NULL; // Already exists
    }

    // Allocate new file node
    fs_node_t *node = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    if (!node)
    {
        return NULL; // Out of memory
    }

    // Initialize file
    memset(node, 0, sizeof(fs_node_t));
    strcpy(node->name, name);
    node->type = type;
    node->size = 0;
    node->created = get_current_time();
    node->modified = node->created;

    // Set function pointers based on type
    if (type == FS_FILE)
    {
        node->read = fs_read;
        node->write = fs_write;
        node->open = NULL;
        node->close = NULL;
        node->finddir = NULL;
        node->mkdir = NULL;
        node->create = NULL;
        node->unlink = NULL;
        node->children = NULL;
        node->child_count = 0;
        node->data = NULL; // Will be allocated on first write
    }
    else if (type == FS_DIRECTORY)
    {
        // This is effectively the same as fs_mkdir, but we allow specifying the type
        node->read = NULL;
        node->write = NULL;
        node->open = NULL;
        node->close = NULL;
        node->finddir = fs_finddir;
        node->mkdir = fs_mkdir;
        node->create = fs_create;
        node->unlink = fs_unlink;

        // Allocate children array
        node->children = (fs_node_t **)kmalloc(sizeof(fs_node_t *) * FS_MAX_DIR_ENTRIES);
        if (!node->children)
        {
            kfree(node);
            return NULL; // Out of memory
        }

        // Clear children array
        memset(node->children, 0, sizeof(fs_node_t *) * FS_MAX_DIR_ENTRIES);
        node->child_count = 0;
    }

    // Set parent
    node->parent = parent;

    // Add to parent's children
    parent->children[parent->child_count++] = node;

    // Update parent modification time
    parent->modified = node->created;

    return node;
}

// Delete a file or directory
int fs_unlink(fs_node_t *parent, char *name)
{
    // Check if parent is a directory
    if (!parent || parent->type != FS_DIRECTORY)
    {
        return -1;
    }

    // Find the node to delete
    fs_node_t *node = NULL;
    int index = -1;

    for (uint32_t i = 0; i < parent->child_count; i++)
    {
        if (strcmp(parent->children[i]->name, name) == 0)
        {
            node = parent->children[i];
            index = i;
            break;
        }
    }

    if (!node || index < 0)
    {
        return -1; // Not found
    }

    // If it's a directory, ensure it's empty
    if (node->type == FS_DIRECTORY && node->child_count > 0)
    {
        return -1; // Directory not empty
    }

    // Free resources
    if (node->type == FS_FILE && node->data)
    {
        kfree(node->data);
    }
    else if (node->type == FS_DIRECTORY && node->children)
    {
        kfree(node->children);
    }

    // Remove from parent's children array
    for (uint32_t i = index; i < parent->child_count - 1; i++)
    {
        parent->children[i] = parent->children[i + 1];
    }
    parent->child_count--;
    parent->modified = get_current_time();

    // Free the node itself
    kfree(node);

    return 0; // Success
}

// Path Manipulation Functions

// Split path into components
static char **split_path(const char *path, int *count)
{
    // Count components
    int num_components = 0;
    int len = strlen(path);

    // Root path is a special case
    if (len == 1 && path[0] == '/')
    {
        *count = 1;
        char **components = (char **)kmalloc(sizeof(char *));
        components[0] = (char *)kmalloc(2);
        strcpy(components[0], "/");
        return components;
    }

    // Count slashes (plus one for the last component if not ending with slash)
    for (int i = 0; i < len; i++)
    {
        if (path[i] == '/' && (i == 0 || path[i - 1] != '/'))
        {
            num_components++;
        }
    }
    if (len > 0 && path[len - 1] != '/')
    {
        num_components++;
    }

    // Allocate component array
    char **components = (char **)kmalloc(sizeof(char *) * num_components);
    if (!components)
    {
        *count = 0;
        return NULL;
    }

    // Extract components
    int start = 0;
    int comp_index = 0;

    for (int i = 0; i <= len; i++)
    {
        if (i == len || path[i] == '/')
        {
            if (i > start)
            {
                int comp_len = i - start;
                components[comp_index] = (char *)kmalloc(comp_len + 1);
                strncpy(components[comp_index], path + start, comp_len);
                components[comp_index][comp_len] = '\0';
                comp_index++;
            }
            else if (i == 0)
            {
                // Root directory special case
                components[comp_index] = (char *)kmalloc(2);
                strcpy(components[comp_index], "/");
                comp_index++;
            }
            start = i + 1;
        }
    }

    *count = comp_index;
    return components;
}

// Resolve a path to a filesystem node
fs_node_t *fs_resolve_path(const char *path)
{
    if (!path || !fs_root)
    {
        return NULL;
    }

    // Handle empty path
    if (!*path)
    {
        return NULL;
    }

    // Handle root directory
    if (strcmp(path, "/") == 0)
    {
        return fs_root;
    }

    // Split path into components
    int count;
    char **components = split_path(path, &count);

    if (!components || count == 0)
    {
        return NULL;
    }

    // Start at root or current directory depending on path
    fs_node_t *current = fs_root;
    int start_index = 0;

    if (strcmp(components[0], "/") == 0)
    {
        start_index = 1; // Skip the root component
    }

    // Traverse the path
    for (int i = start_index; i < count; i++)
    {
        current = fs_finddir(current, components[i]);

        if (!current)
        {
            // Free components
            for (int j = 0; j < count; j++)
            {
                kfree(components[j]);
            }
            kfree(components);
            return NULL; // Path component not found
        }
    }

    // Free components
    for (int i = 0; i < count; i++)
    {
        kfree(components[i]);
    }
    kfree(components);

    return current;
}

// Get the basename of a path
char *fs_basename(const char *path)
{
    if (!path)
    {
        return NULL;
    }

    // Handle empty path
    if (!*path)
    {
        return NULL;
    }

    // Find the last slash
    const char *last_slash = strrchr(path, '/');

    if (!last_slash)
    {
        // No slash, the whole path is the basename
        char *result = (char *)kmalloc(strlen(path) + 1);
        strcpy(result, path);
        return result;
    }

    // Handle root directory
    if (last_slash == path && *(last_slash + 1) == '\0')
    {
        char *result = (char *)kmalloc(2);
        strcpy(result, "/");
        return result;
    }

    // Skip the slash
    last_slash++;

    // Handle empty basename (path ends with slash)
    if (*last_slash == '\0')
    {
        // Find the previous slash or the beginning of the path
        const char *prev_slash = last_slash - 2;
        while (prev_slash >= path && *prev_slash != '/')
        {
            prev_slash--;
        }

        prev_slash++; // Skip the slash or move to the beginning

        size_t len = last_slash - prev_slash - 1;
        char *result = (char *)kmalloc(len + 1);
        strncpy(result, prev_slash, len);
        result[len] = '\0';
        return result;
    }

    // Regular basename (everything after the last slash)
    char *result = (char *)kmalloc(strlen(last_slash) + 1);
    strcpy(result, last_slash);
    return result;
}

// Get the directory name of a path
char *fs_dirname(const char *path)
{
    if (!path)
    {
        return NULL;
    }

    // Handle empty path
    if (!*path)
    {
        return NULL;
    }

    // Handle root directory
    if (strcmp(path, "/") == 0)
    {
        char *result = (char *)kmalloc(2);
        strcpy(result, "/");
        return result;
    }

    // Find the last slash
    const char *last_slash = strrchr(path, '/');

    if (!last_slash)
    {
        // No slash, dirname is current directory
        char *result = (char *)kmalloc(2);
        strcpy(result, ".");
        return result;
    }

    // Handle cases where the last slash is the first character
    if (last_slash == path)
    {
        char *result = (char *)kmalloc(2);
        strcpy(result, "/");
        return result;
    }

    // Regular dirname (everything up to the last slash)
    size_t len = last_slash - path;
    char *result = (char *)kmalloc(len + 1);
    strncpy(result, path, len);
    result[len] = '\0';
    return result;
}