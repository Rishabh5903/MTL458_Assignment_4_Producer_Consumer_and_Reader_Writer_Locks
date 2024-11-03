#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    sem_t mutex;
    sem_t wrt;
    int readCount;
} RWLock;

RWLock* initRWLock() {
    RWLock* rwlock = (RWLock*)malloc(sizeof(RWLock));
    sem_init(&rwlock->mutex, 0, 1);
    sem_init(&rwlock->wrt, 0, 1);
    rwlock->readCount = 0;
    return rwlock;
}

void destroyRWLock(RWLock* rwlock) {
    sem_destroy(&rwlock->mutex);
    sem_destroy(&rwlock->wrt);
    free(rwlock);
}

void *reader(void *arg) {
    char buffer[256];
    FILE *outputFile, *sharedFile;
    RWLock* rwlock = (RWLock*)arg;
    
    // Entry section
    sem_wait(&rwlock->mutex);
    rwlock->readCount++;
    if(rwlock->readCount == 1) {
        sem_wait(&rwlock->wrt);
    }
    // Print status when reader starts
    outputFile = fopen("output-reader-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Reading,Number-of-readers-present:[%d]\n", rwlock->readCount);
        fclose(outputFile);
    }
    
    sem_post(&rwlock->mutex);
    
    // Critical section - Read entire file
    sharedFile = fopen("shared-file.txt", "r");
    if (sharedFile != NULL) {
        while (fgets(buffer, sizeof(buffer), sharedFile)) {
            // Just reading the file
        }
        fclose(sharedFile);
    }

    
    // Exit section
    sem_wait(&rwlock->mutex);
    rwlock->readCount--;
    if(rwlock->readCount == 0) {
        sem_post(&rwlock->wrt);
    }
    sem_post(&rwlock->mutex);
    
    return NULL;
}

void *writer(void *arg) {
    FILE *outputFile, *sharedFile;
    RWLock* rwlock = (RWLock*)arg;
    
    sem_wait(&rwlock->wrt);
    
    outputFile = fopen("output-reader-pref.txt", "a");
    if (outputFile != NULL) {
        fprintf(outputFile, "Writing,Number-of-readers-present:[0]\n");
        fclose(outputFile);
    }
    
    sharedFile = fopen("shared-file.txt", "a");
    if (sharedFile != NULL) {
        fprintf(sharedFile, "Hello world!\n");
        fclose(sharedFile);
    }
    
    sem_post(&rwlock->wrt);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_readers> <num_writers>\n", argv[0]);
        return 1;
    }

    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);
    
    // Initialize the RWLock structure
    RWLock* rwlock = initRWLock();
    
    pthread_t *readers = malloc(num_readers * sizeof(pthread_t));
    pthread_t *writers = malloc(num_writers * sizeof(pthread_t));
    
    if (!readers || !writers) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Create output file
    FILE *outputFile = fopen("output-reader-pref.txt", "w");
    if (outputFile != NULL) {
        fclose(outputFile);
    }
    
    // Create reader threads
    for(int i = 0; i < num_readers; i++) {
        if (pthread_create(&readers[i], NULL, reader, rwlock) != 0) {
            printf("Failed to create reader thread\n");
        }
    }
    
    // Create writer threads
    for(int i = 0; i < num_writers; i++) {
        if (pthread_create(&writers[i], NULL, writer, rwlock) != 0) {
            printf("Failed to create writer thread\n");
        }
    }
    
    // Wait for all threads to complete
    for(int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }
    for(int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }
    
    // Cleanup
    destroyRWLock(rwlock);
    free(readers);
    free(writers);
    
    return 0;
}