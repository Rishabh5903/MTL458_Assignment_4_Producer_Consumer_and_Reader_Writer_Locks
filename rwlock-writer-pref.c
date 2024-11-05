#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Global variables for synchronization
sem_t mutex_readcount, mutex_writecount, mutex_readtry, wrt;
int readCount = 0, writeCount = 0;

void *reader(void *arg) {
    FILE *outputFile, *sharedFile;

    // Entry section
    sem_wait(&mutex_readtry);    // Reader tries to enter
    sem_wait(&mutex_readcount);  // Lock readcount for update

    readCount++;
    if (readCount == 1) {        // If first reader
        sem_wait(&wrt);          // Lock the writer
    }
    // Log the reader count inside the critical section right after updating
    outputFile = fopen("output-writer-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Reading,Number-of-readers-present:[%d]\n", readCount);
        fclose(outputFile);
    }

    sem_post(&mutex_readcount);  // Release readcount lock
    sem_post(&mutex_readtry);    // Done trying to read

    // Critical section - Read entire file
    sharedFile = fopen("shared-file.txt", "r");
    if (sharedFile != NULL) {
        char *buffer = NULL;
        size_t bufferSize = 0;
        ssize_t lineSize;

        while ((lineSize = getline(&buffer, &bufferSize, sharedFile)) != -1) {
            // Process each line if needed
        }

        free(buffer);
        fclose(sharedFile);
    }

    // Exit section
    sem_wait(&mutex_readcount);  // Lock readcount for update
    readCount--;
    if (readCount == 0) {        // If last reader
        sem_post(&wrt);          // Release writer lock
    }
    sem_post(&mutex_readcount);  // Release readcount lock

    return NULL;
}

void *writer(void *arg) {
    FILE *outputFile, *sharedFile;

    // Entry section
    sem_wait(&mutex_writecount); // Lock writecount for update
    writeCount++;
    if (writeCount == 1) {       // If first writer
        sem_wait(&mutex_readtry); // Prevent new readers
    }
    sem_post(&mutex_writecount); // Release writecount lock

    sem_wait(&wrt);             // Wait for readers to finish

    // Critical section - Log writer access
    outputFile = fopen("output-writer-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Writing,Number-of-readers-present:[%d]\n", readCount);
        fclose(outputFile);
    }

    // Critical section - Write to shared file
    sharedFile = fopen("shared-file.txt", "a");
    if (sharedFile != NULL) {
        fprintf(sharedFile, "Hello world!\n");
        fclose(sharedFile);
    }

    // Exit section
    sem_post(&wrt);             // Release writing lock

    sem_wait(&mutex_writecount); // Lock writecount for update
    writeCount--;
    if (writeCount == 0) {       // If last writer
        sem_post(&mutex_readtry); // Allow readers again
    }
    sem_post(&mutex_writecount); // Release writecount lock

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
    sem_init(&mutex_readtry, 0, 1);
    sem_init(&wrt, 0, 1);

    pthread_t readers[num_readers], writers[num_writers];

    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        if (pthread_create(&readers[i], NULL, reader, NULL) != 0) {
            return 1;
        }
    }

    // Create writer threads
    for (int i = 0; i < num_writers; i++) {
        if (pthread_create(&writers[i], NULL, writer, NULL) != 0) {
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
    sem_destroy(&mutex_readtry);
    sem_destroy(&wrt);

    return 0;
}