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
    FILE *out_file;
    FILE *shared_file;
    char buffer[1024];

    sem_wait(&writeBlock);
    sem_wait(&mutex);
    
    // Critical section start
    read_count++;
    out_file = fopen("Part-2/output-writer-pref.txt", "a");
    fprintf(out_file, "Reading,Number-of-readers-present:[%d]\n", read_count);
    fflush(out_file);
    fclose(out_file);
    
    if(read_count == 1) {
        sem_wait(&resource);
    }
    sem_post(&mutex);
    sem_post(&writeBlock);

    // Reading
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

    return NULL;
}

void *writer(void *arg) {
    FILE *out_file;
    FILE *shared_file;

    sem_wait(&writeBlock);
    sem_wait(&resource);

    // Critical section
    out_file = fopen("Part-2/output-writer-pref.txt", "a");
    fprintf(out_file, "Writing,Number-of-readers-present:[0]\n");
    fflush(out_file);
    fclose(out_file);
    
    shared_file = fopen("Part-2/shared-file.txt", "a");
    if(shared_file != NULL) {
        fprintf(shared_file, "Hello world!\n");
        fclose(shared_file);
    }

    sem_post(&resource);
    sem_post(&writeBlock);

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

    // Create reader and writer threads
    for (int i = 0; i < n; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }
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