#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

sem_t mutex;           // Protects read_count
sem_t rw_lock;        // Main lock for writers
int read_count = 0;   // Number of readers currently reading

void *reader(void *arg) {
    FILE *out_file = fopen("Part-2/output-reader-pref.txt", "a");
    FILE *shared_file;
    char buffer[1024];

    // Entry section
    sem_wait(&mutex);
    read_count++;
    fprintf(out_file, "Reading,Number-of-readers-present:[%d]\n", read_count);
    fflush(out_file);
    if(read_count == 1) {
        sem_wait(&rw_lock);
    }
    sem_post(&mutex);

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
        sem_post(&rw_lock);
    }
    sem_post(&mutex);

    fclose(out_file);
    return NULL;
}

void *writer(void *arg) {
    FILE *out_file = fopen("Part-2/output-reader-pref.txt", "a");
    FILE *shared_file;

    sem_wait(&rw_lock);

    // Critical section
    fprintf(out_file, "Writing,Number-of-readers-present:[0]\n");
    fflush(out_file);
    
    shared_file = fopen("Part-2/shared-file.txt", "a");
    if(shared_file != NULL) {
        fprintf(shared_file, "Hello world!\n");
        fclose(shared_file);
    }

    sem_post(&rw_lock);

    fclose(out_file);
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 3) return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    sem_init(&mutex, 0, 1);
    sem_init(&rw_lock, 0, 1);

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
    sem_destroy(&rw_lock);
    return 0;
}