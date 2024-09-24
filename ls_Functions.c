#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <libgen.h>
#include "ls_Functions.h"

#define MAX_ENTRIES 100

// External flags from ls_Functions.h
extern uint8_t is_no_option_enabled;           // Flag to indicate no options specified
extern uint8_t is_long_format_enabled;         // Flag for long format output
extern uint8_t is_hidden_files_enabled;        // Flag to show hidden files
extern uint8_t is_sort_by_time_enabled;        // Flag to sort files by modification time
extern uint8_t is_sort_by_access_time_enabled; // Flag to sort files by last access time (corresponds to '-u')
extern uint8_t is_directory_option_enabled;     // Flag to indicate directory listing
extern uint8_t is_ctime_option_enabled;        // Flag to indicate sorting by change time
extern uint8_t is_no_sort_enabled;             // Flag to disable sorting
extern uint8_t is_inode_enabled;               // Flag to display inode numbers
extern uint8_t is_column_output_enabled;       // Flag for column output format

/**
 * @brief Comparison function for qsort to compare files by their change time (ctime).
 * 
 * This function retrieves the ctime (status change time) of two files and 
 * compares them. It returns -1 if the first file's ctime is more recent, 
 * 1 if it is older, and 0 if both files have the same ctime.
 *
 * @param a Pointer to the first file path (const void* for qsort compatibility).
 * @param b Pointer to the second file path (const void* for qsort compatibility).
 * @return int -1 if the first file is more recent, 1 if older, 0 if the same.
 */
int compare_by_ctime(const void *a, const void *b) {
    struct stat file_stat1, file_stat2;  // File metadata structures

    // Extract file paths from the qsort arguments
    const char *file_path1 = *(const char **)a;
    const char *file_path2 = *(const char **)b;

    // Get file stats for the first file
    if (stat(file_path1, &file_stat1) == -1) {
        perror("stat failed for file_path1"); // Print error if stat fails
        return 1; // Return positive number to indicate error
    }

    // Get file stats for the second file
    if (stat(file_path2, &file_stat2) == -1) {
        perror("stat failed for file_path2"); // Print error if stat fails
        return -1; // Return negative number to indicate error
    }

    // Compare the ctime (status change time) of the two files
    if (file_stat1.st_ctime > file_stat2.st_ctime) {
        return -1; // file_path1 is newer
    } else if (file_stat1.st_ctime < file_stat2.st_ctime) {
        return 1;  // file_path1 is older
    } else {
        return 0;  // Both files have the same ctime
    }
}

/**
 * @brief Comparison function for qsort to sort files by their last access time, newest first.
 * 
 * This function compares the access time (`atime`) of two files and 
 * sorts them in descending order (newest first). If the access time 
 * is the same, it falls back to lexicographical comparison by file name.
 *
 * @param a Pointer to the first file path (const void* for qsort compatibility).
 * @param b Pointer to the second file path (const void* for qsort compatibility).
 * @return int 1 if file a is older, -1 if file a is newer, 0 if access times are the same.
 */
int compare_by_access_time(const void *a, const void *b) {
    struct stat file_stat1, file_stat2;  // File metadata structures

    // Retrieve file stats for the first file
    if (stat(*(const char **)a, &file_stat1) == -1) {
        perror("stat failed for file_stat1"); // Print error message if stat fails
        return 1;  // Return positive value, assume first file is older in case of error
    }

    // Retrieve file stats for the second file
    if (stat(*(const char **)b, &file_stat2) == -1) {
        perror("stat failed for file_stat2"); // Print error message if stat fails
        return -1; // Return negative value, assume second file is older in case of error
    }

    // Compare the access time (st_atime) of both files
    if (file_stat1.st_atime < file_stat2.st_atime) {
        return 1;  // file_stat1 is older, should come later
    } else if (file_stat1.st_atime > file_stat2.st_atime) {
        return -1;  // file_stat1 is newer, should come first
    }

    // If access times are equal, fall back to lexicographical sorting by file name
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * @brief Case-insensitive comparison function for qsort.
 * 
 * This function compares two strings (file names) in a case-insensitive 
 * manner. It converts both strings to lowercase before performing the comparison.
 *
 * @param p1 Pointer to the first string (const void* for qsort compatibility).
 * @param p2 Pointer to the second string (const void* for qsort compatibility).
 * @return int Negative if p1 < p2, positive if p1 > p2, 0 if they are equal.
 */
static int compare_case_insensitive(const void *p1, const void *p2) {
    const char *file_name1 = *(const char **)p1;
    const char *file_name2 = *(const char **)p2;

    // Buffers to store lowercase versions of the file names
    char lower_file_name1[1024], lower_file_name2[1024];

    // Copy original names to temporary buffers
    strcpy(lower_file_name1, file_name1);
    strcpy(lower_file_name2, file_name2);

    // Convert file_name1 to lowercase
    for (int i = 0; lower_file_name1[i]; i++) {
        lower_file_name1[i] = tolower(lower_file_name1[i]);
    }

    // Convert file_name2 to lowercase
    for (int i = 0; lower_file_name2[i]; i++) {
        lower_file_name2[i] = tolower(lower_file_name2[i]);
    }

    // Compare the lowercase versions of the file names
    return strcmp(lower_file_name1, lower_file_name2);
}

/**
 * @brief Comparison function for qsort that prioritizes hidden files ('.' and '..').
 * 
 * This function compares two file names, ensuring that the hidden files `.` and `..` 
 * are always prioritized at the top of the list. For other files, it performs a 
 * standard lexicographical comparison.
 * 
 * @param a Pointer to the first file path (const void* for qsort compatibility).
 * @param b Pointer to the second file path (const void* for qsort compatibility).
 * @return int Negative if a < b, positive if a > b, 0 if they are equal.
 */
int compare_with_hidden(const void *a, const void *b) {
    const char *file_name1 = *(const char **)a;  // Dereference and get the first file name
    const char *file_name2 = *(const char **)b;  // Dereference and get the second file name

    // Special case: prioritize '.' and '..' at the top of the list
    if (strcmp(file_name1, ".") == 0) return -1;  // '.' should come first
    if (strcmp(file_name1, "..") == 0) return -1; // '..' should come second
    if (strcmp(file_name2, ".") == 0) return 1;   // If the second file is '.', it comes first
    if (strcmp(file_name2, "..") == 0) return 1;  // If the second file is '..', it comes first

    // Standard lexicographical comparison for other entries
    return strcmp(file_name1, file_name2);
}

/**
 * @brief Simple lexicographical comparison function for qsort.
 * 
 * This function compares two file names in lexicographical order, without
 * any special handling for hidden files or case sensitivity.
 * 
 * @param a Pointer to the first file path (const void* for qsort compatibility).
 * @param b Pointer to the second file path (const void* for qsort compatibility).
 * @return int Negative if a < b, positive if a > b, 0 if they are equal.
 */
int compare(const void *a, const void *b) {
    // Perform a simple lexicographical comparison of the two file names
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * @brief Prints a file or directory name with appropriate colors based on its type.
 *
 * This function prints file or directory names in color depending on their type:
 * - Blue for directories
 * - Cyan for symbolic links (with special handling to print the link target)
 * - Green for executables
 * - Default color for regular files
 * 
 * It also handles symbolic links, printing the link target and coloring the target 
 * based on its type (directory, executable, etc.).
 *
 * @param path Path to the file or directory.
 */
void print_with_color(char *path) {
    struct stat file_info;        // Structure to hold information about the file/directory
    struct stat target_info;      // Structure to hold information about the symlink target
    char *file_name;              // Base name or full path of the file
    char target_path[256];        // Buffer to store the target of the symbolic link
    ssize_t link_length;          // Length of the symbolic link target path

    // Get the base name of the path (or the full path if necessary)
    file_name = basename(path);

    // Retrieve file or directory information using lstat
    if (lstat(path, &file_info) == -1) {
        perror("Failed to retrieve file information");
        return; // Exit the function if an error occurs
    }

    // If sorting is enabled, determine the type and print in corresponding color
    if (is_no_sort_enabled == 0) {
        if (S_ISDIR(file_info.st_mode)) {
            printf("\033[34m%s\033[0m   ", file_name);  // Blue for directories
        } else if (S_ISLNK(file_info.st_mode)) {
            // Handle symbolic links
            link_length = readlink(path, target_path, sizeof(target_path) - 1); // Get the target of the symlink
            if (link_length != -1 && is_long_format_enabled == 1) {
                target_path[link_length] = '\0'; // Null-terminate the target path

                // Retrieve the type of the symbolic link target
                if (lstat(target_path, &target_info) == -1) {
                    // If target info cannot be retrieved, print the link without coloring the target
                    printf("\033[36m%s\033[0m -> %s   ", file_name, target_path);  // Cyan for the symlink name
                } else {
                    // Print the target with color based on its type
                    if (S_ISDIR(target_info.st_mode)) {
                        printf("\033[36m%s\033[0m -> \033[34m%s\033[0m   ", file_name, target_path);  // Cyan for link, Blue for directory target
                    } else if (target_info.st_mode & S_IXUSR) {
                        printf("\033[36m%s\033[0m -> \033[32m%s\033[0m   ", file_name, target_path);  // Cyan for link, Green for executable target
                    } else {
                        printf("\033[36m%s\033[0m -> %s   ", file_name, target_path);  // Cyan for link, default for regular target
                    }
                }
            } else {
                printf("\033[36m%s\033[0m   ", file_name);  // Cyan for symlink if target retrieval fails
            }
        } else if (file_info.st_mode & S_IXUSR) {
            printf("\033[32m%s\033[0m   ", file_name);  // Green for executables
        } else {
            printf("%s   ", file_name);  // Default color for regular files
        }
    } else {
        // If no sorting is applied, just print the symlink with its target
        link_length = readlink(path, target_path, sizeof(target_path) - 1); // Get the target of the symlink
        if (link_length != -1 && is_long_format_enabled == 1) {
            target_path[link_length] = '\0'; // Null-terminate the target path
            printf("%s -> %s   ", file_name, target_path); // Print the symlink and its target
        } else {
            printf("%s   ", file_name);  // Print the file name if it's not a symlink
        }
    }
}

/**
 * @brief Prints file or directory names in columns with color based on their type.
 *
 * This function is similar to `print_with_color`, but it prints file or directory names in column format,
 * with a new line after each name. The colors used are:
 * - Blue for directories
 * - Cyan for symbolic links (with the target printed after "->")
 * - Green for executables
 * - Default color for regular files
 *
 * It also prints symbolic links with the target and colors the target based on its type (directory, executable, etc.).
 *
 * @param path Path to the file or directory.
 */
void print_column_with_color(char *path) {
    struct stat file_info;        // Structure to hold file or directory information
    struct stat target_info;      // Structure to hold information about the symbolic link target
    char *file_name;              // Base name or full path of the file
    char target_path[256];        // Buffer to store the target of the symbolic link
    ssize_t link_length;          // Length of the symbolic link target

    // Get the base name of the path (or the full path if necessary)
    file_name = basename(path);

    // Retrieve file or directory information using lstat
    if (lstat(path, &file_info) == -1) {
        perror("Failed to retrieve file information");
        return; // Exit the function if an error occurs
    }

    // If sorting is enabled, determine the type and print in corresponding color
    if (is_no_sort_enabled == 0) {
        // Check if it's a directory
        if (S_ISDIR(file_info.st_mode)) {
            printf("\033[34m%s\033[0m\n", file_name);  // Blue for directories
        }
        // Check if it's a symbolic link
        else if (S_ISLNK(file_info.st_mode)) {
            // Get the target of the symbolic link
            link_length = readlink(path, target_path, sizeof(target_path) - 1);
            if (link_length != -1 && is_long_format_enabled == 1) {
                target_path[link_length] = '\0'; // Null-terminate the target path

                // Retrieve the type of the symbolic link target
                if (lstat(target_path, &target_info) == -1) {
                    // If target info cannot be retrieved, print the link without coloring the target
                    printf("\033[36m%s\033[0m -> %s\n", file_name, target_path);  // Cyan for the symlink name
                } else {
                    // Print the target with color based on its type
                    if (S_ISDIR(target_info.st_mode)) {
                        printf("\033[36m%s\033[0m -> \033[34m%s\033[0m\n", file_name, target_path);  // Cyan for link, Blue for directory target
                    } else if (target_info.st_mode & S_IXUSR) {
                        printf("\033[36m%s\033[0m -> \033[32m%s\033[0m\n", file_name, target_path);  // Cyan for link, Green for executable target
                    } else {
                        printf("\033[36m%s\033[0m -> %s\n", file_name, target_path);  // Cyan for link, default for regular target
                    }
                }
            } else {
                // Just print the name if the target of the symbolic link cannot be retrieved
                printf("\033[36m%s\033[0m\n", file_name);  // Cyan for symbolic links
            }
        }
        // Check if it's an executable file
        else if (file_info.st_mode & S_IXUSR) {
            printf("\033[32m%s\033[0m\n", file_name);  // Green for executables
        }
        // For regular files
        else {
            printf("%s\n", file_name);  // Default color for regular files
        }
    }
    // If sorting is disabled, just print the symbolic link and its target
    else {
        // Get the target of the symbolic link
        link_length = readlink(path, target_path, sizeof(target_path) - 1);
        if (link_length != -1 && is_long_format_enabled == 1) {
            target_path[link_length] = '\0';  // Null-terminate the target path
            printf("%s -> %s\n", file_name, target_path);  // Print the symbolic link and its target
        } else {
            // Print the file name if it's not a symbolic link
            printf("%s\n", file_name);
        }
    }
}

/**
 * @brief Lists files in the specified directory, with options for sorting and colorized output.
 *
 * This function lists the files in the directory specified by `input_path`. It supports various options like:
 * - Sorting by access time, change time, or default alphabetical sorting (including handling of hidden files).
 * - Displaying file inodes if enabled.
 * - Colorized output based on file types (directories, symbolic links, executables, etc.).
 *
 * @param input_path Path to the directory or file to list.
 */
void do_ls(char *input_path) {
    struct dirent *directory_entry;        // Pointer for directory entries
    struct stat file_stat;                 // Structure to hold file or directory stats
    uint8_t is_file = 0;                   // Flag to check if the input is a file
    DIR *directory_ptr = opendir(input_path); // Open the directory
    char full_path[1024];                  // Buffer to store the full path for each entry
    char *file_entries[MAX_ENTRIES];       // Array to store the names of files in the directory
    int entry_count = 0;                   // Counter for the number of entries

    // Check if the directory can be opened
    if (directory_ptr == NULL) {
        // If the directory can't be opened, check if it's a regular file
        if (stat(input_path, &file_stat) == -1) {
            perror("stat failed");
            return;
        }
        // Check if the input is a regular file, not a directory
        if (!S_ISREG(file_stat.st_mode)) {
            fprintf(stderr, "Cannot open directory: %s\n", input_path);
            return;
        }
        is_file = 1;  // Set the flag if it's a regular file
    }

    // If input is a directory, proceed to read its entries
    if (is_file == 0) {
        // Read directory entries
        while ((directory_entry = readdir(directory_ptr)) != NULL) {
            // Skip hidden files if the hiddenfiles_flag is not set
            if (directory_entry->d_name[0] == '.' && is_hidden_files_enabled == 0 && is_no_sort_enabled == 0) {
                continue;
            }
            // Store the name of the entry in the file_entries array
            file_entries[entry_count++] = strdup(directory_entry->d_name);
        }
        closedir(directory_ptr);  // Close the directory after reading entries

        // Sort the entries based on the specified sort options
        if ((is_sort_by_time_enabled == 1 || is_sort_by_access_time_enabled == 1) && is_no_sort_enabled == 0) {
            qsort(file_entries, entry_count, sizeof(char *), compare_by_access_time);
        } else if (is_ctime_option_enabled == 1 && is_no_sort_enabled == 0) {
            qsort(file_entries, entry_count, sizeof(char *), compare_by_ctime);
        } else if (is_no_sort_enabled == 0) {
            qsort(file_entries, entry_count, sizeof(char *), compare_with_hidden);
        }

        // Print the sorted entries
        for (int i = 0; i < entry_count; i++) {
            // Build the full path for each entry
            if (strcmp(input_path, "/") == 0) {
                snprintf(full_path, sizeof(full_path), "%s%s", input_path, file_entries[i]);
            } else {
                snprintf(full_path, sizeof(full_path), "%s/%s", input_path, file_entries[i]);
            }

            // Print inode if the inode_flag is set
            if (is_inode_enabled == 1) {
                if (lstat(full_path, &file_stat) != -1) {
                    printf("%6ld ", file_stat.st_ino);  // Print the inode number
                }
            }

            // Print the entry with or without column format based on column_flag
            if (is_column_output_enabled == 1) {
                print_column_with_color(full_path);  // Print in column format with color
            } else {
                print_with_color(full_path);  // Print in standard format with color
            }
            free(file_entries[i]);  // Free the allocated memory for the entry name
        }
    } 
    // If the input was a regular file, handle the file directly
    else {
        // Print inode if the inode_flag is set
        if (is_inode_enabled == 1) {
            if (lstat(input_path, &file_stat) != -1) {
                printf("%6ld ", file_stat.st_ino);  // Print the inode number
            }
        }

        // Print the file with or without column format based on column_flag
        if (is_column_output_enabled == 1) {
            print_column_with_color(input_path);  // Print in column format with color
        } else {
            print_with_color(input_path);  // Print in standard format with color
        }
    }

    // Add a newline if not printing in column format
    if (is_column_output_enabled == 0) {
        printf("\n");  // New line after listing
    }
}
/**
 * @brief Lists the contents of a directory in long format, including details like permissions, owner, size, and modification time.
 *
 * This function lists files and directories within the specified `input_path`, providing detailed information for each entry.
 * It also supports sorting by time, showing hidden files, displaying inode numbers, and summing file sizes.
 *
 * @param input_path Path to the directory or file to list.
 */
void list_directory_long_format(char *input_path) {
    DIR *directory_ptr = opendir(input_path);  // Open the directory
    struct dirent *directory_entry;            // Structure to hold directory entries
    struct stat file_stat;                     // Structure to hold file statistics
    char full_path[1024];                      // Buffer to store the full path for each file
    long total_size = 0;                       // Total size of files in the directory (in bytes)
    char is_file = 0;                          // Flag to check if the input is a file

    // Check if the directory can be opened
    if (directory_ptr == NULL) {
        // If it's not a directory, check if it's a regular file
        if (stat(input_path, &file_stat) == -1) {
            perror("stat failed");
            return;
        }
        // If not a regular file, print an error
        if (!S_ISREG(file_stat.st_mode)) {
            fprintf(stderr, "Cannot open directory: %s\n", input_path);
            return;
        }
        is_file = 1;  // Set the flag indicating the input is a file
    }

    // Array to hold filenames, assuming a max of 1024 entries
    char *filenames[1024];
    int total_count = 0;

    // If it's a directory, read and process its contents
    if (is_file == 0) {
        // First pass: Read entries and store filenames
        while ((directory_entry = readdir(directory_ptr)) != NULL) {
            // Construct the full path for each entry
            if (strcmp(input_path, "/") == 0) {
                snprintf(full_path, sizeof(full_path), "%s%s", input_path, directory_entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s/%s", input_path, directory_entry->d_name);
            }

            // Skip hidden files if the hiddenfiles_flag is not set
            if (directory_entry->d_name[0] == '.' && is_hidden_files_enabled == 0 && is_no_sort_enabled == 0) {
                continue;
            }

            // Store the filename and increment the total count
            filenames[total_count++] = strdup(directory_entry->d_name);

            // Get file statistics and add the size to total_size
            if (stat(full_path, &file_stat) == 0) {
                total_size += file_stat.st_size;  // Add the file size to the total size
            } else {
                perror("stat failed");
                return;
            }
        }

        // Close the directory after reading its contents
        closedir(directory_ptr);

        // Sort the filenames based on flags and options
        if (is_hidden_files_enabled == 1 && is_sort_by_time_enabled == 0 && is_no_sort_enabled == 0) {
            qsort(filenames, total_count, sizeof(char *), compare_with_hidden);
        } else if (is_sort_by_time_enabled == 1 && is_no_sort_enabled == 0) {
            qsort(filenames, total_count, sizeof(char *), compare_by_access_time);
        } else if (is_no_sort_enabled == 0) {
            qsort(filenames, total_count, sizeof(char *), compare_case_insensitive);
        }

        // Print total size in kilobytes (total_size is in bytes)
        printf("total %ld\n", total_size / 1024);

        // Second pass: Display detailed information for each entry
        for (int i = 0; i < total_count; i++) {
            // Construct the full path for each entry
            snprintf(full_path, sizeof(full_path), "%s/%s", input_path, filenames[i]);

            // Print inode number if inode_flag is set
            if (is_inode_enabled == 1) {
                if (lstat(full_path, &file_stat) != -1) {
                    printf("%6ld ", file_stat.st_ino);  // Print inode number
                }
            }

            // Print the file's detailed information in long format
            print_longformat(full_path);
            free(filenames[i]);  // Free the allocated memory for the filename
        }
    } 
    // If the input is a file, process it directly
    else {
        // Print inode number if inode_flag is set
        if (is_inode_enabled == 1) {
            if (lstat(input_path, &file_stat) != -1) {
                printf("%6ld ", file_stat.st_ino);  // Print inode number
            }
        }

        // Print the file's detailed information in long format
        print_longformat(input_path);
    }
}

/**
 * @brief Prints the detailed information of a file or directory in long format.
 *
 * This function retrieves file statistics and displays the file type, permissions, owner,
 * group, size, last modification time, and the name of the file or directory.
 *
 * @param path Path to the file or directory to be printed.
 */
void print_longformat(char *path) {
    struct stat buf;                       // Structure to hold file statistics
    char permissions[10];                  // Buffer for file permissions string
    int mode;                              // Variable to store file mode
    uid_t ownerID;                         // Variable for storing the owner ID
    struct passwd *ownerInfo;              // Structure for user information
    struct group *grp;                     // Structure for group information
    struct tm *modification_time;          // Structure to hold modification time
    char time_str[100];                    // Buffer to store formatted time string

    // Retrieve file stats
    if (lstat(path, &buf) == -1) {
        perror("lstat failed");
        return;
    }

    mode = buf.st_mode;

    // Determine file type and print the corresponding character
    if (S_ISREG(mode)) printf("-");
    else if (S_ISDIR(mode)) printf("d");
    else if (S_ISBLK(mode)) printf("b");
    else if (S_ISCHR(mode)) printf("c");
    else if (S_ISLNK(mode)) printf("l");
    else if (S_ISFIFO(mode)) printf("p");
    else if (S_ISSOCK(mode)) printf("s");
    else printf("Unknown type");

    // Construct permission string
    strcpy(permissions, "---------");

    // Owner permissions
    if (mode & S_IRUSR) permissions[0] = 'r'; // Owner read
    if (mode & S_IWUSR) permissions[1] = 'w'; // Owner write
    if (mode & S_IXUSR) permissions[2] = (mode & S_ISUID) ? 's' : 'x'; // Owner execute or setuid

    // Group permissions
    if (mode & S_IRGRP) permissions[3] = 'r'; // Group read
    if (mode & S_IWGRP) permissions[4] = 'w'; // Group write
    if (mode & S_IXGRP) permissions[5] = (mode & S_ISGID) ? 's' : 'x'; // Group execute or setgid

    // Others permissions
    if (mode & S_IROTH) permissions[6] = 'r'; // Others read
    if (mode & S_IWOTH) permissions[7] = 'w'; // Others write
    if (mode & S_IXOTH) permissions[8] = (mode & S_ISVTX) ? 't' : 'x'; // Others execute or sticky bit

    // Get Owner and Group information
    ownerID = buf.st_uid;
    ownerInfo = getpwuid(ownerID);
    grp = getgrgid(buf.st_gid);
    if (ownerInfo == NULL) {
        perror("getpwuid failed");
        return;
    }
    if (grp == NULL) {
        perror("getgrgid failed");
        return;
    }

    // Get the last modification time
    modification_time = localtime(&buf.st_mtime);
    if (modification_time == NULL) {
        perror("localtime failed");
        return;
    }

    // Set locale for Arabic (or any desired locale)
    if (!setlocale(LC_TIME, "ar_AE.UTF-8")) {
        setlocale(LC_TIME, "C"); // Default to English if Arabic locale is not available
    }

    // Format the time
    if (strftime(time_str, sizeof(time_str), "%A %d %H:%M", modification_time) == 0) {
        printf("Error formatting the time\n");
        return;
    }

    // Print file permissions, number of hard links, owner, group, size, modification time, and name
    printf("%s ", permissions);            // Print permission string
    printf("%-2ld ", buf.st_nlink);         // Print number of hard links
    printf("%6s ", ownerInfo->pw_name);  // Print owner's name
    printf("%6s ", grp->gr_name);         // Print group's name
    printf("%5ld ", buf.st_size);          // Print file size
    printf("%16s ", time_str);                // Print formatted modification time
    print_with_color(path);                 // Print the file/directory name in color
    printf("\n");
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @brief Lists directories or files with specified formatting.
 *
 * This function sorts the provided paths, checks their status, and then
 * prints detailed information based on the flags set for long format,
 * column format, or default format.
 *
 * @param multiArgs Array of paths to directories or files.
 * @param argCount Number of paths in the multiArgs array.
 */
void list_directories(char *multiArgs[], int argCount) {
    struct stat buf; // Structure to hold file statistics

    // Sort the array of paths
    qsort(multiArgs, argCount, sizeof(char *), compare);

    // Iterate over each argument and retrieve its status
    for (int i = 0; i < argCount; i++) {
        // Retrieve file statistics
        if (stat(multiArgs[i], &buf) == -1) {
            perror("stat failed");
            continue; // Skip to the next item if stat fails
        }

        // Print in long format if the flag is set
        if (is_long_format_enabled == 1) {
            print_longformat(multiArgs[i]);
        } else {
            // Print in column format if the column flag is set
            if (is_column_output_enabled == 1) {
                print_column_with_color(multiArgs[i]);
            } else {
                // Default print format
                print_with_color(multiArgs[i]);
            }
        }
    }

    // Print a newline if neither long format nor column format is requested
    if (is_long_format_enabled == 0 && is_column_output_enabled == 0) {
        printf("\n");
    }
}

