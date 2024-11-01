#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

sem_t mutex;           // Protects read_count
sem_t writeBlock;      // Blocks new readers when writers are waiting
sem_t resource;        // Controls access to the resource
int read_count = 0;    // Number of readers currently reading

void *reader(void *arg) {
    FILE *out_file = fopen("Part-2/output-writer-pref.txt", "a");
    FILE *shared_file;
    char buffer[1024];

    // Wait if there are writers
    sem_wait(&writeBlock);
    
    // Entry section
    sem_wait(&mutex);
    if(read_count == 0) {
        sem_wait(&resource);
    }
    read_count++;
    fprintf(out_file, "Reading,Number-of-readers-present:[%d]\n", read_count);
    fflush(out_file);
    sem_post(&mutex);
    sem_post(&writeBlock);

    // Critical section
    shared_file = fopen("Part-2/shared-file.txt", "r");
    if(shared_file != NULL) {
        while(fgets(buffer, sizeof(buffer), shared_file)) {}
        fclose(shared_file);
    }

    // Exit section
    sem_wait(&mutex);
    read_count--;
    if(read_count == 0) {
        sem_post(&resource);
    }
    sem_post(&mutex);

    fclose(out_file);
    return NULL;
}

void *writer(void *arg) {
    FILE *out_file = fopen("Part-2/output-writer-pref.txt", "a");
    FILE *shared_file;

    // Entry section - block readers
    sem_wait(&writeBlock);
    sem_wait(&resource);

    // Critical section
    fprintf(out_file, "Writing,Number-of-readers-present:[0]\n");
    fflush(out_file);
    
    shared_file = fopen("Part-2/shared-file.txt", "a");
    if(shared_file != NULL) {
        fprintf(shared_file, "Hello world!\n");
        fclose(shared_file);
    }

    // Exit section
    sem_post(&resource);
    sem_post(&writeBlock);

    fclose(out_file);
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 3) return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    sem_init(&mutex, 0, 1);
    sem_init(&writeBlock, 0, 1);
    sem_init(&resource, 0, 1);

    // Create reader threads first
    for (int i = 0; i < n; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }
    
    // Then create writer threads
    for (int i = 0; i < m; i++) {
        pthread_create(&writers[i], NULL, writer, NULL);
    }

    // Wait for all threads
    for (int i = 0; i < n; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++) pthread_join(writers[i], NULL);

    sem_destroy(&mutex);
    sem_destroy(&writeBlock);
    sem_destroy(&resource);
    return 0;
}