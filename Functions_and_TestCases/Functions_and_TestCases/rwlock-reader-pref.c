#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

sem_t mutex;   // Protects the readCount variable
sem_t wrt;     // Allows exclusive access to the writer
int readCount = 0;  // Global read count variable

void initRWLock() {
    sem_init(&mutex, 0, 1);
    sem_init(&wrt, 0, 1);
}

void destroyRWLock() {
    sem_destroy(&mutex);
    sem_destroy(&wrt);
}

void *reader(void *arg) {
    FILE *outputFile, *sharedFile;

    // Entry section
    sem_wait(&mutex);
    readCount++;
    if (readCount == 1) {
        sem_wait(&wrt);  // First reader locks the writer
    }

    // Print status when reader starts
    outputFile = fopen("output-reader-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Reading, Number-of-readers-present: [%d]\n", readCount);
        fclose(outputFile);
    }

    sem_post(&mutex);

    // Critical section - Read entire file
    sharedFile = fopen("shared-file.txt", "r");
    if (sharedFile != NULL) {
        char *buffer = NULL;
        size_t bufferSize = 0;
        ssize_t lineSize;

        // Read each line, automatically resizing the buffer as necessary
        while ((lineSize = getline(&buffer, &bufferSize, sharedFile)) != -1) {
            // Process each line here if needed (currently just reading)
        }

        // Free the buffer after use
        free(buffer);
        fclose(sharedFile);
    }

    // Exit section
    sem_wait(&mutex);
    readCount--;
    if (readCount == 0) {
        sem_post(&wrt);  // Last reader unlocks the writer
    }
    sem_post(&mutex);

    return NULL;
}

void *writer(void *arg) {
    FILE *outputFile, *sharedFile;

    sem_wait(&wrt);

    // Print status when writer starts
    outputFile = fopen("output-reader-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Writing, Number-of-readers-present: [%d]\n", readCount);
        fclose(outputFile);
    }

    sharedFile = fopen("shared-file.txt", "a");
    if (sharedFile != NULL) {
        fprintf(sharedFile, "Hello world!\n");
        fclose(sharedFile);
    }

    sem_post(&wrt);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_readers> <num_writers>\n", argv[0]);
        return 1;
    }

    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);

    // Initialize the semaphores
    initRWLock();

    pthread_t *readers = malloc(num_readers * sizeof(pthread_t));
    pthread_t *writers = malloc(num_writers * sizeof(pthread_t));

    if (!readers || !writers) {
        printf("Memory allocation failed\n");
        return 1;
    }


    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        if (pthread_create(&readers[i], NULL, reader, NULL) != 0) {
            printf("Failed to create reader thread\n");
        }
    }

    // Create writer threads
    for (int i = 0; i < num_writers; i++) {
        if (pthread_create(&writers[i], NULL, writer, NULL) != 0) {
            printf("Failed to create writer thread\n");
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
    destroyRWLock();
    free(readers);
    free(writers);

    return 0;
}
