#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BELT_CAPACITY 5
#define MAX_LUGGAGE 20
#define PRODUCER_TIME 2
#define CONSUMER_TIME 4
#define MONITOR_TIME 5

int conveyor_belt[BELT_CAPACITY];
int belt_count = 0;
int insert_index = 0;
int remove_index = 0;
int total_loaded = 0;
int total_dispatched = 0;

// Synchronization primitives
pthread_mutex_t belt_mutex;
pthread_cond_t cond_belt_full;  // Signals producer when space is available
pthread_cond_t cond_belt_empty; // Signals consumer when luggage is available

// Conveyor belt loader 
void* conveyor_loader(void* arg) {
    for (int i = 1; i <= MAX_LUGGAGE; i++) {
        sleep(PRODUCER_TIME);

        // Lock mutex to ensure exclusive access to shared buffer and counters
        pthread_mutex_lock(&belt_mutex);

        // while-loop prevents spurious wakeups when checking full belt condition
        while (belt_count == BELT_CAPACITY) {
            printf("[Loader] Belt is FULL. Waiting for space...\n");
            pthread_cond_wait(&cond_belt_full, &belt_mutex);
        }

        // Shared memory management
        conveyor_belt[insert_index] = i;
        insert_index = (insert_index + 1) % BELT_CAPACITY;
        belt_count++;
        total_loaded++;

        printf("[Loader] Loaded Luggage ID: %d | Current Belt Size: %d\n", i, belt_count);

        // Signal consumer that the belt is no longer empty
        pthread_cond_signal(&cond_belt_empty);
        
        pthread_mutex_unlock(&belt_mutex);
    }
    printf("[Loader] Finished loading all %d luggage items.\n", MAX_LUGGAGE);
    pthread_exit(NULL);
}

// Aircraft loader
void* aircraft_loader(void* arg) {
    int dispatched_count = 0;
    
    while (dispatched_count < MAX_LUGGAGE) {
        // Lock mutex to safely read/modify shared state
        pthread_mutex_lock(&belt_mutex);

        // while-loop prevents spurious wakeups when checking empty belt condition
        while (belt_count == 0) {
            printf("[Aircraft] Belt is EMPTY. Waiting for luggage...\n");
            pthread_cond_wait(&cond_belt_empty, &belt_mutex);
        }

        // Shared memory management
        int luggage_id = conveyor_belt[remove_index];
        remove_index = (remove_index + 1) % BELT_CAPACITY;
        belt_count--;
        
        printf("[Aircraft] Removed Luggage ID: %d from belt | Current Belt Size: %d\n", luggage_id, belt_count);

        // Signal producer that space is now available on the belt
        pthread_cond_signal(&cond_belt_full);
        
        pthread_mutex_unlock(&belt_mutex);

        sleep(CONSUMER_TIME);
        
        // Re-acquire lock to safely update the dispatched counter after processing
        pthread_mutex_lock(&belt_mutex);
        total_dispatched++;
        dispatched_count = total_dispatched;
        printf("[Aircraft] Dispatched Luggage ID: %d into aircraft.\n", luggage_id);
        pthread_mutex_unlock(&belt_mutex);
    }
    printf("[Aircraft] Finished dispatching all %d luggage items.\n", MAX_LUGGAGE);
    pthread_exit(NULL);
}

// Monitoring thread
void* system_monitor(void* arg) {
    int active = 1;
    
    while (active) {
        sleep(MONITOR_TIME);

        // Locks mutex to prevent reading partially updated states
        pthread_mutex_lock(&belt_mutex);
        
        printf("\n-- [MONITOR REPORT] --\n");
        printf("Total Luggage Loaded onto Belt: %d\n", total_loaded);
        printf("Total Luggage Dispatched to Aircraft: %d\n", total_dispatched);
        printf("Current Belt Size: %d / %d\n", belt_count, BELT_CAPACITY);
        printf("------------------------\n\n");

        if (total_dispatched == MAX_LUGGAGE) {
            active = 0; 
        }

        pthread_mutex_unlock(&belt_mutex);
    }
    pthread_exit(NULL);
}

// Main function
int main() {
    pthread_t producer_thread, consumer_thread, monitor_thread;

    pthread_mutex_init(&belt_mutex, NULL);
    pthread_cond_init(&cond_belt_full, NULL);
    pthread_cond_init(&cond_belt_empty, NULL);

    printf("Starting Airport Baggage Handling Simulation...\n\n");

    pthread_create(&producer_thread, NULL, conveyor_loader, NULL);
    pthread_create(&consumer_thread, NULL, aircraft_loader, NULL);
    pthread_create(&monitor_thread, NULL, system_monitor, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    pthread_join(monitor_thread, NULL);

    pthread_mutex_destroy(&belt_mutex);
    pthread_cond_destroy(&cond_belt_full);
    pthread_cond_destroy(&cond_belt_empty);

    printf("\nSimulation Complete.\n");
    return 0;
}
