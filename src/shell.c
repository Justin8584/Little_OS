// shell.c - Updated with filesystem commands

#include "shell.h"
#include "fb.h"        // For fb_write_string, fb_write_dec, fb_clear, colors
#include "multiboot.h" // For meminfo command (accessing MB info)
#include "common.h"    // For uintN_t, size_t, NULL
#include "string.h"    // For strcmp, strlen, strchr, strncpy, strcpy, strstr
#include "fs.h"        // For filesystem nodes and functions
#include "pmm.h"       // For pmm_memory_map in meminfo (optional)
#include "kmalloc.h"   // For kmalloc/kfree, kheap_stats (optional)

// Access the global MB info address (defined in kmain.c)
extern unsigned long global_mb_info_addr; // Assuming this is set in kmain.c

// --- Shell State ---
char cmd_buffer[CMD_BUFFER_SIZE]; // Defined in shell.h, allocated here
int cmd_buffer_idx = 0;

// Current working directory node
fs_node_t *current_dir = NULL; // Needs to be initialized in shell_init

// --- Utility Functions ---

// Function to clear the command buffer (used by keyboard handler and command runner)
void clear_cmd_buffer()
{
    memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
    cmd_buffer_idx = 0;
}

// Function to print an error message in red
void fb_write_error(const char *msg)
{
    fb_write_string("Error: ", FB_RED, FB_BLACK);
    fb_write_string(msg, FB_RED, FB_BLACK);
    fb_write_string("\n", FB_RED, FB_BLACK);
}

// NOTE: fb_write_dec should be defined in fb.c and declared in fb.h
//       Ensure it's NOT defined here.

// --- Command Handlers ---

// 'cat' command - Display file contents
void cmd_cat(const char *args)
{
    if (!args || !*args)
    {
        fb_write_error("No filename specified");
        return;
    }

    // Resolve the path relative to current_dir or absolute
    char full_path[FS_MAX_PATH];
    if (args[0] != '/')
    {
        // Relative path logic would go here if needed, simpler to use absolute for now
        // or implement path joining. For simplicity, assume absolute or relative to root.
        // For this example, let's use fs_resolve_path which handles root-based paths
        // or paths starting from the provided node (which isn't passed here).
        // A proper shell needs better path handling. Let's assume fs_resolve handles it from root if needed.
        // THIS IS A SIMPLIFICATION - a real shell needs proper relative path handling based on 'current_dir'
        strcpy(full_path, args); // Simplistic approach
    }
    else
    {
        strcpy(full_path, args);
    }

    fs_node_t *node = fs_resolve_path(full_path); // Pass the combined path
    if (!node)
    {
        fb_write_string("Debug: Path='", FB_WHITE, FB_BLACK);
        fb_write_string(full_path, FB_WHITE, FB_BLACK);
        fb_write_string("'\n", FB_WHITE, FB_BLACK);
        fb_write_error("File not found");
        return;
    }

    // Check if it's a file
    if (node->type != FS_FILE)
    {
        fb_write_error("Not a file");
        return;
    }

    // Read and display the file contents
    if (node->size == 0)
    {
        fb_write_string("(empty file)\n", FB_WHITE, FB_BLACK);
        return;
    }

    // Allocate buffer (+1 for null terminator, although fs_read doesn't null terminate)
    uint8_t *buffer = (uint8_t *)kmalloc(node->size + 1);
    if (!buffer)
    {
        fb_write_error("Out of memory");
        return;
    }

    uint32_t bytes_read = fs_read(node, 0, node->size, buffer);
    buffer[bytes_read] = '\0'; // Null-terminate *after* reading

    fb_write_string((char *)buffer, FB_WHITE, FB_BLACK);
    // Ensure a newline if the file doesn't end with one for cleaner shell output
    if (bytes_read > 0 && buffer[bytes_read - 1] != '\n')
    {
        fb_write_string("\n", FB_WHITE, FB_BLACK);
    }

    kfree(buffer);
}

// 'touch' command - Create or update a file
void cmd_touch(const char *args)
{
    if (!args || !*args)
    {
        fb_write_error("No filename specified");
        return;
    }

    // Get the parent directory and file name
    // Need a temporary buffer because fs_dirname/fs_basename might return pointers
    // into the original string depending on implementation, or allocated strings.
    char *dirname = fs_dirname(args);
    char *basename = fs_basename(args);

    if (!dirname || !basename)
    {
        fb_write_error("Invalid path");
        if (dirname)
            kfree(dirname); // fs_dirname/basename now allocate memory
        if (basename)
            kfree(basename);
        return;
    }

    // Find the parent directory based on dirname
    // This resolution needs to handle relative paths correctly based on 'current_dir'
    // For simplicity, we'll resolve from root if it starts with '/', otherwise from current_dir
    fs_node_t *parent;
    if (strcmp(dirname, ".") == 0)
    {
        parent = current_dir;
    }
    else if (strcmp(dirname, "/") == 0 && strcmp(basename, "/") != 0)
    { // Special case for root dir
        parent = fs_root;
    }
    else
    {
        parent = fs_resolve_path(dirname); // Resolve dirname path
    }

    if (!parent)
    {
        fb_write_error("Parent directory not found");
        kfree(dirname);
        kfree(basename);
        return;
    }
    if (parent->type != FS_DIRECTORY)
    {
        fb_write_error("Parent path is not a directory");
        kfree(dirname);
        kfree(basename);
        return;
    }

    // Check if the file already exists within the resolved parent directory
    fs_node_t *node = fs_finddir(parent, basename);

    if (node)
    {
        if (node->type != FS_FILE)
        {
            fb_write_error("Path exists but is not a file");
        }
        else
        {
            // File exists. The filesystem's internal write/create functions handle timestamps.
            // We don't need to call get_current_time() here directly.
            // A more complete 'touch' might involve an fs_utime() call if implemented.
            fb_write_string("File exists: ", FB_GREEN, FB_BLACK);
            fb_write_string(args, FB_GREEN, FB_BLACK);
            fb_write_string("\n", FB_GREEN, FB_BLACK);
        }
    }
    else
    {
        // Create a new file in the resolved parent directory
        node = fs_create(parent, basename, FS_FILE);
        if (!node)
        {
            fb_write_error("Failed to create file");
        }
        else
        {
            fb_write_string("File created: ", FB_GREEN, FB_BLACK);
            fb_write_string(args, FB_GREEN, FB_BLACK);
            fb_write_string("\n", FB_GREEN, FB_BLACK);
        }
    }

    kfree(dirname); // Free allocated memory
    kfree(basename);
}

// 'mkdir' command - Create a directory
void cmd_mkdir(const char *args)
{
    if (!args || !*args)
    {
        fb_write_error("No directory name specified");
        return;
    }

    char *dirname = fs_dirname(args);
    char *basename = fs_basename(args);

    if (!dirname || !basename)
    {
        fb_write_error("Invalid path");
        if (dirname)
            kfree(dirname);
        if (basename)
            kfree(basename);
        return;
    }

    fs_node_t *parent;
    if (strcmp(dirname, ".") == 0)
    {
        parent = current_dir;
    }
    else if (strcmp(dirname, "/") == 0 && strcmp(basename, "/") != 0)
    {
        parent = fs_root;
    }
    else
    {
        parent = fs_resolve_path(dirname);
    }

    if (!parent)
    {
        fb_write_error("Parent directory not found");
        kfree(dirname);
        kfree(basename);
        return;
    }
    if (parent->type != FS_DIRECTORY)
    {
        fb_write_error("Parent path is not a directory");
        kfree(dirname);
        kfree(basename);
        return;
    }

    // Check if it already exists
    if (fs_finddir(parent, basename))
    {
        fb_write_error("Directory or file already exists");
        kfree(dirname);
        kfree(basename);
        return;
    }

    // Create the directory
    fs_node_t *dir = fs_mkdir(parent, basename); // Use fs_mkdir specifically
    if (!dir)
    {
        fb_write_error("Failed to create directory");
    }
    else
    {
        fb_write_string("Directory created: ", FB_GREEN, FB_BLACK);
        fb_write_string(args, FB_GREEN, FB_BLACK);
        fb_write_string("\n", FB_GREEN, FB_BLACK);
    }

    kfree(dirname);
    kfree(basename);
}

// 'rm' command - Remove a file or directory
void cmd_rm(const char *args)
{
    if (!args || !*args)
    {
        fb_write_error("No path specified");
        return;
    }

    char *dirname = fs_dirname(args);
    char *basename = fs_basename(args);

    if (!dirname || !basename)
    {
        fb_write_error("Invalid path");
        if (dirname)
            kfree(dirname);
        if (basename)
            kfree(basename);
        return;
    }
    if (strcmp(basename, ".") == 0 || strcmp(basename, "..") == 0)
    {
        fb_write_error("Cannot remove '.' or '..'");
        kfree(dirname);
        kfree(basename);
        return;
    }

    fs_node_t *parent;
    if (strcmp(dirname, ".") == 0)
    {
        parent = current_dir;
    }
    else if (strcmp(dirname, "/") == 0 && strcmp(basename, "/") != 0)
    {
        parent = fs_root;
    }
    else
    {
        parent = fs_resolve_path(dirname);
    }

    if (!parent)
    {
        fb_write_error("Parent directory not found");
        kfree(dirname);
        kfree(basename);
        return;
    }
    if (parent->type != FS_DIRECTORY)
    {
        fb_write_error("Parent path is not a directory");
        kfree(dirname);
        kfree(basename);
        return;
    }

    // Check if the file/directory exists (using fs_finddir, not fs_unlink)
    fs_node_t *node = fs_finddir(parent, basename);
    if (!node)
    {
        fb_write_error("File or directory not found");
        kfree(dirname);
        kfree(basename);
        return;
    }

    // If it's a directory, fs_unlink will check if it's empty internally
    // Remove the file/directory
    if (fs_unlink(parent, basename) != 0)
    {
        // fs_unlink likely failed because the directory wasn't empty
        fb_write_error("Failed to remove (directory might not be empty)");
    }
    else
    {
        fb_write_string("Removed: ", FB_GREEN, FB_BLACK);
        fb_write_string(args, FB_GREEN, FB_BLACK);
        fb_write_string("\n", FB_GREEN, FB_BLACK);
    }

    kfree(dirname);
    kfree(basename);
}

// 'ls' command - List directory contents
void cmd_ls(const char *args)
{
    fs_node_t *dir_to_list;

    if (!args || !*args)
    {
        dir_to_list = current_dir; // List current directory if no args
    }
    else
    {
        dir_to_list = fs_resolve_path(args); // Resolve path if args provided
        if (!dir_to_list)
        {
            fb_write_error("Directory not found");
            return;
        }
    }

    // Check if it's actually a directory
    if (dir_to_list->type != FS_DIRECTORY)
    {
        // If it's a file, just print its name and size
        fb_write_string("[FILE] ", FB_WHITE, FB_BLACK);
        fb_write_string(dir_to_list->name, FB_WHITE, FB_BLACK);
        fb_write_string(" (", FB_WHITE, FB_BLACK);
        fb_write_dec(dir_to_list->size);
        fb_write_string(" bytes)\n", FB_WHITE, FB_BLACK);
        return;
        // Or fb_write_error("Not a directory"); if you prefer
    }

    // Print header
    // A more robust pwd would be better here
    fb_write_string("Contents:\n", FB_CYAN, FB_BLACK);

    if (dir_to_list->child_count == 0)
    {
        fb_write_string("  (empty directory)\n", FB_WHITE, FB_BLACK);
        return;
    }

    // Print '.' and '..' implicitly for navigation awareness
    fb_write_string("  [DIR] .\n", FB_LIGHT_BLUE, FB_BLACK);
    if (dir_to_list != fs_root)
    { // Root's parent is itself
        fb_write_string("  [DIR] ..\n", FB_LIGHT_BLUE, FB_BLACK);
    }

    // Print each entry with type and size
    for (uint32_t i = 0; i < dir_to_list->child_count; i++)
    {
        fs_node_t *node = dir_to_list->children[i];
        fb_write_string("  ", FB_WHITE, FB_BLACK); // Indent

        // Print type indicator
        if (node->type == FS_DIRECTORY)
        {
            fb_write_string("[DIR] ", FB_LIGHT_BLUE, FB_BLACK);
        }
        else
        {
            fb_write_string("[FILE] ", FB_WHITE, FB_BLACK);
        }

        // Print name
        fb_write_string(node->name, node->type == FS_DIRECTORY ? FB_LIGHT_BLUE : FB_WHITE, FB_BLACK);

        // Print size for files
        if (node->type == FS_FILE)
        {
            fb_write_string(" (", FB_WHITE, FB_BLACK);
            fb_write_dec(node->size);
            fb_write_string(" bytes)", FB_WHITE, FB_BLACK);
        }

        fb_write_string("\n", FB_WHITE, FB_BLACK);
    }
}

// 'cd' command - Change current directory
void cmd_cd(const char *args)
{
    fs_node_t *target_dir;

    if (!args || !*args || strcmp(args, "/") == 0)
    {
        target_dir = fs_root; // Go to root if no args or "/"
    }
    else
    {
        target_dir = fs_resolve_path(args); // Resolve the path
        if (!target_dir)
        {
            fb_write_error("Directory not found");
            return;
        }
        if (target_dir->type != FS_DIRECTORY)
        {
            fb_write_error("Not a directory");
            return;
        }
    }

    // Change current directory
    current_dir = target_dir;
    // Optional: Print new directory for confirmation
    // fb_write_string("Changed to directory: \n", FB_GREEN, FB_BLACK);
    // cmd_pwd(); // Show the new path
}

// 'pwd' command - Print working directory
void cmd_pwd()
{
    if (!current_dir)
    { // Should not happen after init
        fb_write_error("Current directory not set");
        return;
    }
    if (current_dir == fs_root)
    {
        fb_write_string("/\n", FB_WHITE, FB_BLACK);
        return;
    }

    // Build path by traversing up the directory tree
    char path_buffer[FS_MAX_PATH];
    char temp_buffer[FS_MAX_PATH]; // Build backwards here
    memset(path_buffer, 0, FS_MAX_PATH);
    memset(temp_buffer, 0, FS_MAX_PATH);

    fs_node_t *node = current_dir;
    char *ptr = temp_buffer + FS_MAX_PATH - 1; // Start from the end
    *ptr = '\0';                               // Null terminate

    while (node != fs_root)
    {
        int name_len = strlen(node->name);
        ptr -= name_len;
        if (ptr < temp_buffer)
        {
            fb_write_error("Path too long");
            return;
        } // Bounds check
        memcpy(ptr, node->name, name_len);

        ptr--;
        if (ptr < temp_buffer)
        {
            fb_write_error("Path too long");
            return;
        } // Bounds check
        *ptr = '/';

        node = node->parent;
    }

    // If ptr is still at the end, means we were in root somehow (should be caught earlier)
    // or the path is just '/', add the leading slash
    if (ptr == temp_buffer + FS_MAX_PATH - 1)
    {
        ptr--;
        *ptr = '/';
    }

    // Now ptr points to the start of the constructed path string in temp_buffer
    strcpy(path_buffer, ptr); // Copy the result to the final buffer

    fb_write_string(path_buffer, FB_WHITE, FB_BLACK);
    fb_write_string("\n", FB_WHITE, FB_BLACK);
}

// 'echo' command (updated to allow redirecting to files)
void cmd_echo(const char *args)
{
    if (!args)
    {
        fb_write_string("\n", FB_LIGHT_BROWN, FB_BLACK); // Just print newline if no args
        return;
    }

    // Simple echo for now, enhance later if needed
    // Find redirection operators '>' or '>>'
    const char *redirect_op = NULL;
    const char *append_op = NULL;
    int is_append = 0;

    // Search for the operators
    redirect_op = strstr(args, ">");
    append_op = strstr(args, ">>");

    const char *operator_pos = NULL;
    const char *filename_start = NULL;
    size_t text_len = strlen(args); // Default length if no operator found

    // Determine which operator is present and where it is
    if (append_op && (!redirect_op || append_op < redirect_op))
    {
        // Found ">>" first
        operator_pos = append_op;
        filename_start = operator_pos + 2; // Skip ">>"
        text_len = operator_pos - args;
        is_append = 1;
    }
    else if (redirect_op)
    {
        // Found ">" first (or only ">")
        operator_pos = redirect_op;
        filename_start = operator_pos + 1; // Skip ">"
        text_len = operator_pos - args;
        is_append = 0;
    }

    // Handle redirection if an operator was found
    if (operator_pos)
    {
        // Trim trailing whitespace from the text part
        while (text_len > 0 && (args[text_len - 1] == ' ' || args[text_len - 1] == '\t'))
        {
            text_len--;
        }

        // Skip leading whitespace for the filename
        while (*filename_start == ' ' || *filename_start == '\t')
        {
            filename_start++;
        }

        if (!*filename_start)
        {
            fb_write_error("No filename specified for redirection");
            return;
        }

        // Allocate buffer for the text to write (+1 for potential newline, +1 for null)
        char *text_to_write = (char *)kmalloc(text_len + 2);
        if (!text_to_write)
        {
            fb_write_error("Out of memory");
            return;
        }
        strncpy(text_to_write, args, text_len);
        text_to_write[text_len] = '\n'; // Add newline before writing
        text_to_write[text_len + 1] = '\0';

        // Resolve filename (relative/absolute needs work, similar to touch/mkdir)
        char *dirname = fs_dirname(filename_start);
        char *basename = fs_basename(filename_start);
        if (!dirname || !basename)
        { /* Error handling */
            kfree(text_to_write);
            return;
        }

        fs_node_t *parent;
        if (strcmp(dirname, ".") == 0)
            parent = current_dir;
        else if (strcmp(dirname, "/") == 0 && strcmp(basename, "/") != 0)
            parent = fs_root;
        else
            parent = fs_resolve_path(dirname);

        if (!parent || parent->type != FS_DIRECTORY)
        { /* Error handling */
            kfree(text_to_write);
            kfree(dirname);
            kfree(basename);
            return;
        }

        fs_node_t *node = fs_finddir(parent, basename);

        if (!node)
        { // File doesn't exist, create it
            node = fs_create(parent, basename, FS_FILE);
            if (!node)
            { /* Error handling */
                kfree(text_to_write);
                kfree(dirname);
                kfree(basename);
                return;
            }
        }
        else if (node->type != FS_FILE)
        { // Exists, but not a file
            fb_write_error("Cannot redirect to a directory");
            kfree(text_to_write);
            kfree(dirname);
            kfree(basename);
            return;
        }

        kfree(dirname);
        kfree(basename);

        // Write to the file
        uint32_t offset = is_append ? node->size : 0; // Append or overwrite
        uint32_t text_write_len = strlen(text_to_write);
        uint32_t written = fs_write(node, offset, text_write_len, (uint8_t *)text_to_write);

        if (written != text_write_len)
        {
            fb_write_error("Failed to write to file");
        }
        else
        {
            // Optionally print confirmation
        }

        kfree(text_to_write);
    }
    else
    {
        // No redirection, just print the arguments to the screen
        fb_write_string(args, FB_LIGHT_BROWN, FB_BLACK);
        fb_write_string("\n", FB_LIGHT_BROWN, FB_BLACK);
    }
}

// --- Command Execution ---

// Function to execute commands
void run_shell_command(const char *command)
{
    // Basic parsing: command is the first word, rest are args
    char cmd[CMD_BUFFER_SIZE] = {0};
    const char *args = NULL;

    // Find the first space
    const char *space = strchr(command, ' ');
    size_t cmd_len;

    if (space)
    {
        cmd_len = space - command;
        if (cmd_len >= CMD_BUFFER_SIZE)
            cmd_len = CMD_BUFFER_SIZE - 1; // Prevent overflow
        strncpy(cmd, command, cmd_len);
        cmd[cmd_len] = '\0';

        // Arguments start after the space, skipping any extra spaces
        args = space + 1;
        while (*args == ' ' || *args == '\t')
        {
            args++;
        }
        if (*args == '\0')
        { // Handle case like "ls "
            args = NULL;
        }
    }
    else
    {
        // No space, the whole command is the command name
        strncpy(cmd, command, CMD_BUFFER_SIZE - 1);
        cmd[CMD_BUFFER_SIZE - 1] = '\0'; // Ensure null termination
        args = NULL;
    }

    // Execute the appropriate command
    if (strcmp(cmd, "help") == 0)
    {
        fb_write_string("Available commands:\n", FB_GREEN, FB_BLACK);
        fb_write_string("  help       - Show this help message\n", FB_WHITE, FB_BLACK);
        fb_write_string("  cls        - Clear the screen\n", FB_WHITE, FB_BLACK);
        fb_write_string("  echo [...] - Print text; > file or >> file redirects\n", FB_WHITE, FB_BLACK);
        fb_write_string("  meminfo    - Show basic memory info\n", FB_WHITE, FB_BLACK);
        fb_write_string("  ls [path]  - List directory contents\n", FB_WHITE, FB_BLACK);
        fb_write_string("  cd <path>  - Change current directory\n", FB_WHITE, FB_BLACK);
        fb_write_string("  pwd        - Print working directory\n", FB_WHITE, FB_BLACK);
        fb_write_string("  cat <file> - Display file contents\n", FB_WHITE, FB_BLACK);
        fb_write_string("  touch <file>- Create file or update timestamp\n", FB_WHITE, FB_BLACK);
        fb_write_string("  mkdir <dir>- Create a new directory\n", FB_WHITE, FB_BLACK);
        fb_write_string("  rm <path>  - Remove a file or empty directory\n", FB_WHITE, FB_BLACK);
    }
    else if (strcmp(cmd, "cls") == 0)
    {
        fb_clear();
    }
    else if (strcmp(cmd, "echo") == 0)
    {
        cmd_echo(args); // Pass rest of the string as args
    }
    else if (strcmp(cmd, "meminfo") == 0)
    {
        if (global_mb_info_addr == 0)
        {
            fb_write_error("Multiboot info not available.");
            return;
        }
        multiboot_info_t *mb_info = (multiboot_info_t *)global_mb_info_addr;
        if (mb_info->flags & MULTIBOOT_INFO_MEMORY)
        {
            fb_write_string("Mem Lower: ", FB_CYAN, FB_BLACK);
            fb_write_dec(mb_info->mem_lower);
            fb_write_string(" KB\n", FB_CYAN, FB_BLACK);
            fb_write_string("Mem Upper: ", FB_CYAN, FB_BLACK);
            fb_write_dec(mb_info->mem_upper);
            fb_write_string(" KB\n", FB_CYAN, FB_BLACK);
        }
        else
        {
            fb_write_string("Basic memory info not provided by bootloader.\n", FB_LIGHT_BROWN, FB_BLACK);
        }
        // Add pmm_memory_map() or kheap_stats() calls here if desired
        pmm_memory_map();
        kheap_stats();
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        cmd_ls(args);
    }
    else if (strcmp(cmd, "cd") == 0)
    {
        cmd_cd(args);
    }
    else if (strcmp(cmd, "pwd") == 0)
    {
        cmd_pwd();
    }
    else if (strcmp(cmd, "cat") == 0)
    {
        cmd_cat(args);
    }
    else if (strcmp(cmd, "touch") == 0)
    {
        cmd_touch(args);
    }
    else if (strcmp(cmd, "mkdir") == 0)
    {
        cmd_mkdir(args);
    }
    else if (strcmp(cmd, "rm") == 0)
    {
        cmd_rm(args);
    }
    else if (strlen(command) > 0) // Check if it wasn't just an empty input
    {
        fb_write_string("Unknown command: '", FB_RED, FB_BLACK);
        fb_write_string(command, FB_RED, FB_BLACK); // Use original command string here
        fb_write_string("'\n", FB_RED, FB_BLACK);
    }
    // If command was empty, do nothing.
}

// --- Shell Initialization and Running ---

// Initialize shell state
void shell_init()
{
    clear_cmd_buffer();
    if (fs_root)
    {                          // Ensure filesystem was initialized
        current_dir = fs_root; // Set current directory to root
    }
    else
    {
        fb_write_error("Filesystem not initialized, cannot start shell.");
        // Perhaps halt here?
        current_dir = NULL;
    }
}

// Start the shell (display prompt and wait for interrupts)
void shell_run()
{
    if (!current_dir)
        return; // Don't run if init failed

    fb_write_string("> ", FB_CYAN, FB_BLACK); // Show initial prompt
    // The main processing loop is now driven by keyboard interrupts.
    // The kernel can idle here using 'hlt'.
    while (1)
    {
        asm volatile("hlt"); // Halt CPU until the next interrupt occurs
    }
}