#ifndef myls
#define myls
// Function declarations
void print_with_color(char *path);
void print_column_with_color(char *path);
void do_ls(char *input_path);
void list_directory_long_format(char *input_path);
void print_longformat(char *path);
void list_directories(char *multiArgs[], int argCount);
int compare(const void *a, const void *b);
int compare_with_hidden(const void *a, const void *b);
int compare_by_access_time(const void *a, const void *b);
int compare_by_ctime(const void *a, const void *b);
#endif
