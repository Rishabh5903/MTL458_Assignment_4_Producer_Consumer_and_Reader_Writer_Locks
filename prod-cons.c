#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#define BUFFER_SIZE 100

// Circular buffer structure
typedef struct {
    unsigned int buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    bool producer_finished;  // New flag to track producer status
} CircularBuffer;

// Global variables
CircularBuffer cb;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_consume = PTHREAD_COND_INITIALIZER;
FILE *output_file;

// Initialize the circular buffer
void init_buffer() {
    cb.head = 0;
    cb.tail = 0;
    cb.count = 0;
    cb.producer_finished = false;
}

// Add item to buffer
void produce(unsigned int item) {
    pthread_mutex_lock(&mutex);
    
    while (cb.count == BUFFER_SIZE) {
        pthread_cond_wait(&can_produce, &mutex);
    }
    
    cb.buffer[cb.tail] = item;
    cb.tail = (cb.tail + 1) % BUFFER_SIZE;
    cb.count++;
    
    pthread_cond_signal(&can_consume);
    pthread_mutex_unlock(&mutex);
}

// Remove item from buffer
unsigned int consume() {
    pthread_mutex_lock(&mutex);
    
    while (cb.count == 0) {
        // If producer is done and buffer is empty, return 0
        if (cb.producer_finished) {
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        pthread_cond_wait(&can_consume, &mutex);
    }
    
    unsigned int item = cb.buffer[cb.head];
    cb.head = (cb.head + 1) % BUFFER_SIZE;
    cb.count--;
    
    pthread_cond_signal(&can_produce);
    pthread_mutex_unlock(&mutex);
    
    return item;
}

// Print buffer state to file
void print_buffer_state(unsigned int consumed) {
    pthread_mutex_lock(&mutex);
    fprintf(output_file, "Consumed:[%u],Buffer State:[", consumed);
    
    if (cb.count > 0) {
        int current = cb.head;
        for (int i = 0; i < cb.count; i++) {
            fprintf(output_file, "%u", cb.buffer[current]);
            if (i < cb.count - 1) {
                fprintf(output_file, ",");
            }
            current = (current + 1) % BUFFER_SIZE;
        }
    }
    fprintf(output_file, "]\n");
    fflush(output_file);  // Ensure output is written immediately
    pthread_mutex_unlock(&mutex);
}

// Producer thread function
void* producer(void* arg) {
    FILE *input_file = fopen("input-part1.txt", "r");
    if (!input_file) {
        perror("Error opening input file");
        exit(1);
    }
    
    unsigned int num;
    while (fscanf(input_file, "%u", &num) == 1) {
        if (num == 0) break;
        produce(num);
    }
    
    pthread_mutex_lock(&mutex);
    cb.producer_finished = true;  // Signal that producer is done
    pthread_cond_broadcast(&can_consume);  // Wake up consumer to check finish condition
    pthread_mutex_unlock(&mutex);
    
    fclose(input_file);
    return NULL;
}

// Consumer thread function
void* consumer(void* arg) {
    while (1) {
        unsigned int consumed_item = consume();
        
        // Check if we're done
        if (consumed_item == 0 && cb.producer_finished && cb.count == 0) {
            break;
        }
        
        if (consumed_item > 0) {  // Only print if we actually consumed something
            print_buffer_state(consumed_item);
        }
    }
    return NULL;
}

int main() {
    // Initialize buffer
    init_buffer();
    
    // Open output file
    output_file = fopen("output-part1.txt", "w");
    if (!output_file) {
        perror("Error opening output file");
        return 1;
    }
    
    // Create threads
    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    
    // Wait for threads to finish
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    // Clean up
    fclose(output_file);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    
    return 0;
}