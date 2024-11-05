#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_BUFFER_SIZE 100
#define TEST_SIZE 500
#define MAX_LINE_LENGTH 1024

typedef struct {
    int consumed;
    int buffer[MAX_BUFFER_SIZE];
    int buffer_size;
} OutputState;

// Function to compare two arrays
bool arrays_equal(int *arr1, int *arr2, int size) {
    for(int i = 0; i < size; i++) {
        if(arr1[i] != arr2[i]) return false;
    }
    return true;
}

// Function to monitor when new inputs are added to buffer
void monitor_buffer_additions(OutputState *states, int state_count) {
    printf("\n=== Producer Activity Monitor ===\n");
    
    int prev_buffer[MAX_BUFFER_SIZE];
    int prev_size = 0;
    
    for(int i = 0; i < state_count; i++) {
        // Check if new elements were added to buffer
        if(states[i].buffer_size > prev_size || 
           !arrays_equal(states[i].buffer, prev_buffer, prev_size)) {
            
            // Count how many new elements
            int new_elements = 0;
            for(int j = 0; j < states[i].buffer_size; j++) {
                bool found = false;
                for(int k = 0; k < prev_size; k++) {
                    if(states[i].buffer[j] == prev_buffer[k]) {
                        found = true;
                        break;
                    }
                }
                if(!found) new_elements++;
            }
            
            if(new_elements > 0) {
                printf("\nProducer added %d new inputs to buffer\n", new_elements);
                printf("Current buffer size: %d\n", states[i].buffer_size);
                printf("Buffer contents: [");
                for(int j = 0; j < states[i].buffer_size; j++) {
                    printf("%d", states[i].buffer[j]);
                    if(j < states[i].buffer_size - 1) printf(",");
                }
                printf("]\n--------------------\n");
            }
        }
        
        // Update previous state
        memcpy(prev_buffer, states[i].buffer, sizeof(int) * states[i].buffer_size);
        prev_size = states[i].buffer_size;
    }
    
    printf("\n=== End Producer Monitor ===\n\n");
}

void generate_input() {
    FILE *fp = fopen("input-part1.txt", "w");
    if (!fp) {
        printf("Error creating input file!\n");
        exit(1);
    }

    srand(time(NULL));
    for (int i = 0; i < TEST_SIZE; i++) {
        fprintf(fp, "%d\n", rand() + 1);
    }
    fprintf(fp, "0\n");
    fclose(fp);
}

bool parse_buffer_state(char *buffer_str, int *buffer, int *size) {
    *size = 0;
    char *token = strtok(buffer_str, ",");
    while (token != NULL && *size < MAX_BUFFER_SIZE) {
        buffer[(*size)++] = atoi(token);
        token = strtok(NULL, ",");
    }
    return *size <= MAX_BUFFER_SIZE;
}

int read_output(OutputState *states, int max_states) {
    FILE *fp = fopen("output-part1.txt", "r");
    if (!fp) {
        printf("Error opening output file!\n");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int state_count = 0;
    
    while (fgets(line, MAX_LINE_LENGTH, fp) && state_count < max_states) {
        char *consumed_start = strstr(line, "Consumed:[");
        char *buffer_start = strstr(line, "Buffer State:[");
        
        // if (!consumed_start || !buffer_start) {
        //     printf("Invalid output format in line: %s\n", line);
        //     continue;
        // }

        sscanf(consumed_start, "Consumed:[%d]", &states[state_count].consumed);

        char buffer_str[MAX_LINE_LENGTH];
        char *start = buffer_start + strlen("Buffer State:[");
        char *end = strchr(start, ']');
        if (end) {
            strncpy(buffer_str, start, end - start);
            buffer_str[end - start] = '\0';
            if (!parse_buffer_state(buffer_str, states[state_count].buffer, 
                                   &states[state_count].buffer_size)) {
                printf("Invalid buffer state in line: %s\n", line);
                continue;
            }
        }

        state_count++;
    }

    fclose(fp);
    return state_count;
}

bool validate_output(OutputState *states, int state_count) {
    FILE *fp = fopen("input-part1.txt", "r");
    if (!fp) {
        printf("Error opening input file for validation!\n");
        return false;
    }

    int input[TEST_SIZE];
    int input_count = 0;
    int num;
    while (fscanf(fp, "%d", &num) == 1 && num != 0) {
        input[input_count++] = num;
    }
    fclose(fp);

    for (int i = 0; i < input_count; i++) {
        if (i >= state_count || states[i].consumed != input[i]) {
            printf("Error: Consumed sequence doesn't match input sequence at position %d\n", i);
            printf("Expected: %d, Got: %d\n", input[i], states[i].consumed);
            return false;
        }
    }

    for (int i = 0; i < state_count; i++) {
        if (states[i].buffer_size > MAX_BUFFER_SIZE) {
            printf("Error: Buffer size exceeds maximum at step %d\n", i);
            return false;
        }
    }

    return true;
}

int main() {
    // Generate test input
    generate_input();
    
    // Compile and run the producer-consumer program
    system("gcc -o prod-cons prod-cons.c -lpthread");
    system("./prod-cons");
    
    // Read and validate output
    OutputState states[TEST_SIZE];
    int state_count = read_output(states, TEST_SIZE);
    
    if (state_count == 0) {
        printf("No output found or output file couldn't be read!\n");
        return 1;
    }
    
    // Monitor buffer additions
    monitor_buffer_additions(states, state_count);
    
    if (validate_output(states, state_count)) {
        printf("All tests passed successfully!\n");
        printf("Total operations validated: %d\n", state_count);
    } else {
        printf("Validation failed!\n");
        return 1;
    }
    
    return 0;
}