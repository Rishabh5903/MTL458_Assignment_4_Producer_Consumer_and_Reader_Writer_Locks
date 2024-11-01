#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
    sem_t lock;      // binary semaphore (basic lock)
    sem_t writelock; // allow ONE writer/MANY readers
    int readers;     // #readers in critical section
} rwlock_t;

rwlock_t rw_lock;

void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    sem_init(&rw->lock, 0, 1);
    sem_init(&rw->writelock, 0, 1);
}

void rwlock_acquire_readlock(rwlock_t *rw) {
    FILE *out_file;
    
    sem_wait(&rw->lock);
    rw->readers++;
    
    // Print status right after incrementing, while still holding the lock
    out_file = fopen("Part-2/output-reader-pref.txt", "a");
    fprintf(out_file, "Reading,Number-of-readers-present:[%d]\n", rw->readers);
    fclose(out_file);
    
    if (rw->readers == 1)  // first reader gets writelock
        sem_wait(&rw->writelock);
    sem_post(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)  // last reader lets it go
        sem_post(&rw->writelock);
    sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw) {
    sem_wait(&rw->writelock);
}

void rwlock_release_writelock(rwlock_t *rw) {
    sem_post(&rw->writelock);
}

void *reader(void *arg) {
    FILE *shared_file;
    char ch;
    
    // Acquire read lock and print count (now handled in rwlock_acquire_readlock)
    rwlock_acquire_readlock(&rw_lock);
    
    // Read the shared file
    shared_file = fopen("Part-2/shared-file.txt", "r");
    if(shared_file != NULL) {
        while((ch = fgetc(shared_file)) != EOF) {
            // Just read the file
        }
        fclose(shared_file);
    }
    
    // Release read lock
    rwlock_release_readlock(&rw_lock);
    
    return NULL;
}

void *writer(void *arg) {
    FILE *out_file;
    FILE *shared_file;
    
    // Acquire write lock
    rwlock_acquire_writelock(&rw_lock);
    
    // Print status when writer starts
    out_file = fopen("Part-2/output-reader-pref.txt", "a");
    fprintf(out_file, "Writing,Number-of-readers-present:[0]\n");
    fclose(out_file);
    
    // Write to shared file
    shared_file = fopen("Part-2/shared-file.txt", "a");
    if(shared_file != NULL) {
        fprintf(shared_file, "Hello world!\n");
        fclose(shared_file);
    }
    
    // Release write lock
    rwlock_release_writelock(&rw_lock);
    
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 3) return 1;
    
    int n = atoi(argv[1]); // Number of readers
    int m = atoi(argv[2]); // Number of writers
    
    pthread_t readers[n], writers[m];
    
    // Initialize the reader-writer lock
    rwlock_init(&rw_lock);
    
    // Create all reader threads first
    for(int i = 0; i < n; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
        usleep(100);  // Small delay to ensure sequential creation
    }
    
    // Then create writer threads
    for(int i = 0; i < m; i++) {
        pthread_create(&writers[i], NULL, writer, NULL);
    }
    
    // Wait for all threads to complete
    for(int i = 0; i < n; i++) {
        pthread_join(readers[i], NULL);
    }
    for(int i = 0; i < m; i++) {
        pthread_join(writers[i], NULL);
    }
    
    return 0;
}