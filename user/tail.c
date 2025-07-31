/*
 * tail command implementation for xv6
 * Displays the last N lines of a file or stdin
 * 
 * Supported usage patterns:
 * - tail                    (last 10 lines from stdin)
 * - tail file               (last 10 lines from file)
 * - tail -n N               (last N lines from stdin)
 * - tail -n N file          (last N lines from file)
 * - tail -N                 (shorthand for -n N from stdin)
 * - tail -N file            (shorthand for -n N from file)
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_LINE_LENGTH 256
#define DEFAULT_LINES 10
#define BUFFER_SIZE 512

// Global buffer for reading input
char read_buffer[BUFFER_SIZE];

// Check if a string represents a valid positive number
int is_valid_number(char *str) {
    if (!str || *str == '\0') return 0;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    return 1;
}

// Parse command line arguments and return file descriptor and line count
void parse_arguments(int argc, char *argv[], int *fd, int *num_lines) {
    *fd = 0;  // Default to stdin
    *num_lines = DEFAULT_LINES;  // Default to 10 lines
    
    if (argc == 1) {
        // tail - read from stdin
        return;
    }
    
    if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("Usage: tail [-n N] [file]\n");
            printf("Print the last N lines of input (default: 10)\n");
            printf("\nExamples:\n");
            printf("  tail file.txt           # last 10 lines of file.txt\n");
            printf("  tail -n 5 file.txt      # last 5 lines of file.txt\n");
            printf("  cat file.txt | tail     # last 10 lines from pipe\n");
            printf("  tail -3                 # last 3 lines from stdin\n");
            exit(0);
        }
        
        // Check if it's a shorthand option like -5
        if (argv[1][0] == '-' && is_valid_number(argv[1] + 1)) {
            *num_lines = atoi(argv[1] + 1);
            return;
        }
        
        // Otherwise it's a filename
        *fd = open(argv[1], 0);
        if (*fd < 0) {
            fprintf(2, "tail: cannot open %s\n", argv[1]);
            exit(1);
        }
        return;
    }
    
    if (argc == 3) {
        if (strcmp(argv[1], "-n") == 0) {
            // tail -n N
            if (!is_valid_number(argv[2])) {
                fprintf(2, "tail: invalid number of lines: %s\n", argv[2]);
                exit(1);
            }
            *num_lines = atoi(argv[2]);
            return;
        }
        
        // Check if it's shorthand like: tail -5 file
        if (argv[1][0] == '-' && is_valid_number(argv[1] + 1)) {
            *num_lines = atoi(argv[1] + 1);
            *fd = open(argv[2], 0);
            if (*fd < 0) {
                fprintf(2, "tail: cannot open %s\n", argv[2]);
                exit(1);
            }
            return;
        }
        
        fprintf(2, "tail: invalid arguments\n");
        fprintf(2, "Usage: tail [-n N] [file]\n");
        exit(1);
    }
    
    if (argc == 4) {
        if (strcmp(argv[1], "-n") == 0) {
            // tail -n N file
            if (!is_valid_number(argv[2])) {
                fprintf(2, "tail: invalid number of lines: %s\n", argv[2]);
                exit(1);
            }
            *num_lines = atoi(argv[2]);
            *fd = open(argv[3], 0);
            if (*fd < 0) {
                fprintf(2, "tail: cannot open %s\n", argv[3]);
                exit(1);
            }
            return;
        }
    }
    
    fprintf(2, "tail: too many arguments\n");
    fprintf(2, "Usage: tail [-n N] [file]\n");
    exit(1);
}

// Main tail function - reads input and displays last N lines
void tail_lines(int fd, int num_lines) {
    if (num_lines <= 0) {
        return;  // Nothing to display
    }
    
    // Allocate circular buffer for storing lines
    char **line_buffer = malloc(num_lines * sizeof(char*));
    if (!line_buffer) {
        fprintf(2, "tail: memory allocation failed\n");
        exit(1);
    }
    
    // Initialize line buffer
    for (int i = 0; i < num_lines; i++) {
        line_buffer[i] = malloc(MAX_LINE_LENGTH);
        if (!line_buffer[i]) {
            fprintf(2, "tail: memory allocation failed\n");
            exit(1);
        }
        line_buffer[i][0] = '\0';
    }
    
    int current_line = 0;
    int total_lines = 0;
    int buffer_pos = 0;
    int line_pos = 0;
    int bytes_read;
    
    // Read input and store lines in circular buffer
    while ((bytes_read = read(fd, read_buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            char c = read_buffer[i];
            
            if (c == '\n') {
                // End of line - store it in circular buffer
                line_buffer[current_line][line_pos] = '\0';
                current_line = (current_line + 1) % num_lines;
                total_lines++;
                line_pos = 0;
            } else if (line_pos < MAX_LINE_LENGTH - 1) {
                // Add character to current line
                line_buffer[current_line][line_pos++] = c;
            }
            // If line is too long, we just ignore extra characters
        }
    }
    
    // Handle case where input doesn't end with newline
    if (line_pos > 0) {
        line_buffer[current_line][line_pos] = '\0';
        total_lines++;
    }
    
    // Calculate how many lines to actually print
    int lines_to_print = (total_lines < num_lines) ? total_lines : num_lines;
    
    // Calculate starting position in circular buffer
    int start_pos;
    if (total_lines <= num_lines) {
        start_pos = 0;
    } else {
        start_pos = current_line;
    }
    
    // Print the last N lines
    for (int i = 0; i < lines_to_print; i++) {
        int pos = (start_pos + i) % num_lines;
        if (line_buffer[pos][0] != '\0') {
            printf("%s\n", line_buffer[pos]);
        }
    }
    
    // Free allocated memory
    for (int i = 0; i < num_lines; i++) {
        free(line_buffer[i]);
    }
    free(line_buffer);
}

int main(int argc, char *argv[]) {
    int fd, num_lines;
    
    // Parse command line arguments
    parse_arguments(argc, argv, &fd, &num_lines);
    
    // Execute tail functionality
    tail_lines(fd, num_lines);
    
    // Close file if it was opened
    if (fd > 0) {
        close(fd);
    }
    
    exit(0);
}