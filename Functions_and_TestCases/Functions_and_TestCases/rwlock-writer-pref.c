#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Global variables for synchronization
sem_t mutex_readcount;    // Protects readCount
sem_t mutex_writecount;   // Protects writeCount
sem_t write_pref;        // Writers preference control
sem_t resource;          // Protects the actual resource
int readCount = 0;
int writeCount = 0;

void *reader(void *arg) {
    FILE *outputFile, *sharedFile;

    // Try to enter critical section
    sem_wait(&write_pref);     // Check if any writer is waiting
    sem_wait(&mutex_readcount); // Lock readcount for update

    // Enter critical section
    readCount++;
    if (readCount == 1) {
        sem_wait(&resource);    // First reader locks the resource
    }

    // Log reader entry in writer preference output file
    outputFile = fopen("output-writer-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Reading,Number-of-readers-present:[%d]\n", readCount);
        fclose(outputFile);
    }

    sem_post(&mutex_readcount); // Release readcount lock
    sem_post(&write_pref);      // Release writer preference check

    // Reading section
    sharedFile = fopen("shared-file.txt", "r");
    if (sharedFile != NULL) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), sharedFile) != NULL) {
            // Process read data if needed
        }
        fclose(sharedFile);
    }

    // Exit section
    sem_wait(&mutex_readcount);  // Lock readcount for update
    readCount--;
    if (readCount == 0) {        // If last reader
        sem_post(&resource);     // Release the resource
    }
    sem_post(&mutex_readcount);  // Release readcount lock

    return NULL;
}

void *writer(void *arg) {
    FILE *outputFile, *sharedFile;

    // Entry section
    sem_wait(&mutex_writecount);
    writeCount++;
    if (writeCount == 1) {       // First writer
        sem_wait(&write_pref);   // Block all new readers
    }
    sem_post(&mutex_writecount);

    // Wait for exclusive access
    sem_wait(&resource);

    // Log writer entry in writer preference output file
    outputFile = fopen("output-writer-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Writing,Number-of-readers-present:[%d]\n", readCount);
        fclose(outputFile);
    }

    // Write to shared file
    sharedFile = fopen("shared-file.txt", "a");
    if (sharedFile != NULL) {
        fprintf(sharedFile, "Hello world!\n");
        fclose(sharedFile);
    }

    // Exit section
    sem_post(&resource);

    sem_wait(&mutex_writecount);
    writeCount--;
    if (writeCount == 0) {       // Last writer
        sem_post(&write_pref);   // Allow readers again
    }
    sem_post(&mutex_writecount);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_readers> <num_writers>\n", argv[0]);
        return 1;
    }

    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);

    // Initialize semaphores
    sem_init(&mutex_readcount, 0, 1);
    sem_init(&mutex_writecount, 0, 1);
    sem_init(&write_pref, 0, 1);
    sem_init(&resource, 0, 1);

    pthread_t readers[num_readers], writers[num_writers];

    // Create writer threads first to demonstrate preference
    for (int i = 0; i < num_writers; i++) {
        if (pthread_create(&writers[i], NULL, writer, NULL) != 0) {
            perror("Failed to create writer thread");
            return 1;
        }
    }

    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        if (pthread_create(&readers[i], NULL, reader, NULL) != 0) {
            perror("Failed to create reader thread");
            return 1;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    // Cleanup
    sem_destroy(&mutex_readcount);
    sem_destroy(&mutex_writecount);
    sem_destroy(&write_pref);
    sem_destroy(&resource);

    return 0;
}