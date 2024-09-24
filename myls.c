#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/stat.h>
#include "ls_Functions.h" // Header file for function declarations

#define MAX_ARGS 100 // Maximum number of command-line arguments

// Global flags for command-line options
uint8_t is_long_format_enabled = 0;      // Flag for long format output
uint8_t is_no_option_enabled = 0;        // Flag to indicate no options specified
uint8_t is_hidden_files_enabled = 0;     // Flag to show hidden files
uint8_t is_sort_by_time_enabled = 0;     // Flag to sort files by modification time
uint8_t is_sort_by_access_time_enabled = 0; // Flag to sort files by last access time (corresponds to '-u')
uint8_t is_directory_option_enabled = 0; // Flag to indicate directory listing
uint8_t is_ctime_option_enabled = 0;     // Flag to indicate sorting by change time
uint8_t is_no_sort_enabled = 0;          // Flag to disable sorting
uint8_t is_inode_enabled = 0;             // Flag to display inode numbers
uint8_t is_column_output_enabled = 0;     // Flag for column output format

// Function to check if the provided path is a directory
int is_directory(const char *file_path) {
    struct stat file_stat; // Structure to hold file status information
    stat(file_path, &file_stat); // Get the status of the file at the provided path
    return S_ISDIR(file_stat.st_mode); // Return 1 if it is a directory, 0 otherwise
}

// Function to sort and display files and directories
void sort_and_display(char *file_paths[], int argument_count) {
    char *regular_files[MAX_ARGS];      // Array to hold regular file paths
    char *directories[MAX_ARGS];        // Array to hold directory paths
    int regular_file_count = 0;         // Counter for regular files
    int directory_count = 0;            // Counter for directories

    // Separate regular files and directories
    for (int i = 0; i < argument_count; i++) {
        if (is_directory(file_paths[i])) {
            directories[directory_count++] = file_paths[i]; // Store directory path
        } else {
            regular_files[regular_file_count++] = file_paths[i]; // Store regular file path
        }
    }

    // Sort regular files and directories alphabetically
    qsort(regular_files, regular_file_count, sizeof(char *), compare);
    qsort(directories, directory_count, sizeof(char *), compare);

    // Print regular files first
    for (int i = 0; i < regular_file_count; i++) {
        if (is_long_format_enabled == 1) {
            list_directory_long_format(regular_files[i]); // Long format display
        } else if (is_no_option_enabled == 1 || is_hidden_files_enabled == 1) {
            do_ls(regular_files[i]); // Standard display
        }
    }

    // Print directories after regular files
    for (int i = 0; i < directory_count; i++) {
        // Print directory name if there are files or multiple arguments
        if (regular_file_count != 0 || argument_count > 1) {
            printf("\n%s:\n", directories[i]);
        }
        if (is_long_format_enabled == 1) {
            list_directory_long_format(directories[i]); // Long format display for directories
        } else if (is_no_option_enabled == 1 || is_hidden_files_enabled == 1) {
            do_ls(directories[i]); // Standard display for directories
        }
    }
}


int main(int argc, char *argv[]) {
    uint8_t opt_flag = 0;                          // Flag to track options provided
    char *multiArgs[MAX_ARGS];                     // Array to hold command-line arguments
    uint8_t argCount = 0;                          // Counter for the number of arguments
    int opt;                                       // Variable to store the current option
    char buffer[100];                              // Buffer to hold the current directory path
    char *directory;                               // Pointer to the current directory path
    directory = getcwd(buffer, sizeof(buffer));   // Get current working directory

    // If no arguments are provided
    if (argc == 1) {
        printf("Directory listing of pwd:\n");
        do_ls(directory);                          // List the contents of the current directory
    } else {
        // Process command-line options
        while ((opt = getopt(argc, argv, "lautdcfi1")) != -1) {
            is_no_option_enabled = 1;              // Set flag indicating options have been processed
            switch (opt) {
                case 'l':
                    is_long_format_enabled = 1;    // Enable long format
                    break;
                case 'a':
                    is_hidden_files_enabled = 1;   // Include hidden files
                    break;
                case 't':
                    is_sort_by_time_enabled = 1;   // Sort by modification time
                    // Fall through to the next case
                case 'u':
                    is_sort_by_access_time_enabled = 1; // Sort by last access time
                    break;
                case 'd':
                    is_directory_option_enabled = 1; // Indicate directory-only listing
                    // Fall through to the next case
                case 'c':
                    is_ctime_option_enabled = 1;    // Sort by change time
                    break;
                case 'f':
                    is_no_sort_enabled = 1;         // Disable sorting
                    break;
                case 'i':
                    is_inode_enabled = 1;           // Print inode numbers
                    break;
                case '1':
                    is_column_output_enabled = 1;   // Print in single-column format
                    break;
                case '?':
                    // Handle invalid options
                    fprintf(stderr, "Usage: %s [-l [directory1 [directory2 ...]]]\n", argv[0]);
                    break;
                default:
                    printf("Unexpected case in switch()\n");
                    break;
            }
        }

        // If no options are provided (opt_flag == 0)
        if (is_no_option_enabled == 0) {
            uint8_t i = 1; // Start after program name
            argCount = 0; // Reset argument count
            is_no_option_enabled = 1; // Set flag for options
            // Collect all arguments from argv
            while (argv[i] != NULL) {
                if (argCount < MAX_ARGS) {
                    multiArgs[argCount++] = argv[i]; // Store argument
                }
                i++;
            }

            // Sort and display the collected arguments (files and directories)
            sort_and_display(multiArgs, argCount);
        } else {
            // Collect all arguments following the options
            while (optind < argc && argv[optind][0] != '-') {
                if (argCount < MAX_ARGS) {
                    multiArgs[argCount++] = argv[optind++]; // Store argument
                } else {
                    fprintf(stderr, "Too many arguments for option -l\n");
                    return EXIT_FAILURE; // Exit if too many arguments are provided
                }
            }
            // Conditional logic based on flags and argument count
            if (argCount == 0 && is_long_format_enabled == 1 && is_directory_option_enabled == 0) {
                list_directory_long_format(directory); // Use default directory
            } else if (argCount > 0 && is_long_format_enabled == 1 && is_directory_option_enabled == 0) {
                // Sort and display the collected arguments (files and directories)
                sort_and_display(multiArgs, argCount);
            } else if (argCount == 0 && is_hidden_files_enabled == 1 && is_directory_option_enabled == 0) {
                do_ls(directory); // Use default directory
            } else if (argCount > 0 && is_hidden_files_enabled == 1 && is_directory_option_enabled == 0) {
                // Sort and display the collected arguments (files and directories)
                sort_and_display(multiArgs, argCount);
            } else if (argCount == 0 && (is_sort_by_time_enabled == 1 || is_ctime_option_enabled == 1 || is_sort_by_access_time_enabled == 1 || is_no_sort_enabled == 1 || is_inode_enabled == 1 || is_column_output_enabled == 1) && is_directory_option_enabled == 0) {
                do_ls(directory); // Use default directory
            } else if ((argCount > 0 && (is_sort_by_time_enabled == 1 || is_ctime_option_enabled == 1 || is_sort_by_access_time_enabled == 1 || is_no_sort_enabled == 1 || is_inode_enabled == 1 || is_column_output_enabled == 1) && is_directory_option_enabled == 0)) {
                // Sort and display the collected arguments (files and directories)
                sort_and_display(multiArgs, argCount);
            } else if (argCount == 0 && is_directory_option_enabled == 1) {
                // Handle the case where only the directory flag is set
                if (is_no_sort_enabled == 1) {
                    printf(".\n"); // Print current directory
                } else {
                    print_with_color("."); // Print current directory with color
                    printf("\n");
                }
            } else if (argCount > 0 && is_directory_option_enabled == 1) {
                list_directories(multiArgs, argCount); // List specified directories
            }
        }
    }

    return 0; // Exit program successfully
}

