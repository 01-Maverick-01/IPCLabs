/****************************************************************************************/
/* CSE 5441 - Lab assignment 2                                                          */
/*      This program implements a multi threaded version of producer-consumer problem   */
/*      using pthreads library. The producer is reposible for populating the queue and  */
/*      consumer reads this queue and displays output.                                  */
/*      There are in total 16 Producer threads and 16 consumer threads. Once, producer  */
/*      has added to the queue, it will trigger a signal to let the consumer know that  */
/*      it is safe to read from queue and consumers will do that same after reading all */
/*      items from the queue.
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Compile using 'icc' compiler                                                         */
/* Created on Feb 18 2020                                                               */
/*                                                                                      */
/****************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 5
#define PRODUCER_THREAD_COUNT 16
#define CONSUMER_THREAD_COUNT 16

#define THREAD_POOL(NAME, SIZE)     struct                          \
                                    {                               \
                                        int pool[SIZE];             \
                                        int addItemIndex;           \
                                        int popItemIndex;           \
                                        int count;                  \
                                        double totalTime;           \
                                    }NAME;                          \

uint16_t transformA(uint16_t input_val);
uint16_t transformB(uint16_t input_val);
uint16_t transformC(uint16_t input_val);
uint16_t transformD(uint16_t input_val);

struct work_entry
{
    char cmd;
    uint16_t key;
};

struct Locks
{
    pthread_mutex_t modify_queue_mutex;
    pthread_mutex_t producer_read_intput_mutex;
    pthread_mutex_t consumer_display_output_mutex;
    pthread_mutex_t clock_mutex;
    pthread_cond_t producer_writeq_cond; 
    pthread_cond_t consumer_readq_cond;
    pthread_cond_t consumer_display_cond;
};

struct State
{
    struct Locks locks;
    int numItems;
    struct work_entry items[SIZE];
    int stopProducers;
    THREAD_POOL(producers, PRODUCER_THREAD_COUNT)
    THREAD_POOL(consumers, CONSUMER_THREAD_COUNT)
};

struct ThreadArg
{
    int threadId;
    struct State *state;
};

// thread safe method to add item to queue
void addItemsToJobQueue(struct State *state, struct work_entry itemsToAdd[SIZE], int numItemsToAdd, int threadId)
{
    if (numItemsToAdd == 0)                                     // if not items to add, then return
        return;
    
    pthread_mutex_lock(&state->locks.modify_queue_mutex);       // acquire queue modification lock

    // if queue is already full or if it is not current thread's turn to add to queue, then wait
    while (state->numItems == SIZE || state->producers.pool[state->producers.popItemIndex] != threadId)
        pthread_cond_wait(&state->locks.producer_writeq_cond, &state->locks.modify_queue_mutex);

    for (int i = 0; i < numItemsToAdd; i++)                     // add items to queue
    {
        state->items[i].cmd = itemsToAdd[i].cmd;
        state->items[i].key = itemsToAdd[i].key;
        state->numItems++;
    }

    // update id of next thread that is expected to add to queue
    state->producers.pool[state->producers.popItemIndex] = -1;
    state->producers.popItemIndex = (state->producers.popItemIndex + 1) % PRODUCER_THREAD_COUNT;
    
    pthread_cond_broadcast(&state->locks.consumer_readq_cond);  // notify consumer that it is safe to read from queue
    pthread_mutex_unlock(&state->locks.modify_queue_mutex);     // release queue modification lock
}

// thread safe method to read items from queue
void getItemsFromJobQueue(struct State *state, struct work_entry itemsFound[SIZE], int* numItemsFound, int threadId)
{
    pthread_mutex_lock(&state->locks.modify_queue_mutex);       // acquire queue modification lock

    while (state->numItems == 0 && state->producers.count != 0) // if queue is empty, then wait
        pthread_cond_wait(&state->locks.consumer_readq_cond, &state->locks.modify_queue_mutex);
    
    *numItemsFound = state->numItems;
    for (int i = 0; i < *numItemsFound; i++)                    // read items from queue
    {
        itemsFound[i].cmd = state->items[i].cmd;
        itemsFound[i].key = state->items[i].key;
        state->numItems--;
    }
    state->consumers.pool[state->consumers.addItemIndex] = threadId;
    state->consumers.addItemIndex = (state->consumers.addItemIndex + 1) % CONSUMER_THREAD_COUNT;
    
    pthread_cond_broadcast(&state->locks.producer_writeq_cond); // notify producers that it is safe to add to queue
    pthread_mutex_unlock(&state->locks.modify_queue_mutex);     // // release queue modification lock
}

// Producer routine
void *producer(void *arg)
{
    struct ThreadArg* threadArg = (struct ThreadArg *)arg;              // parse input args
    int threadId = threadArg->threadId;
    struct State *state = threadArg->state;
    state->producers.count++;
    
    time_t startTime = time(NULL);                                      //Get new time
    while (state->stopProducers != 1)
    {
        char cmdArr[SIZE];
        uint16_t ekey, keyArr[SIZE];
        struct work_entry tempItems[SIZE];
        int numItems = 0;
        
        pthread_mutex_lock(&state->locks.producer_read_intput_mutex);           // acquire lock to prevent other threads from reading input
        
        state->producers.pool[state->producers.addItemIndex] = threadId;        // update thread id, so that we know which thread should write to queue
        state->producers.addItemIndex = (state->producers.addItemIndex + 1) % PRODUCER_THREAD_COUNT;

        while(numItems < SIZE && state->stopProducers != 1)
        {
            scanf("%c  %hu\n", &cmdArr[numItems], &keyArr[numItems]);
            if (cmdArr[numItems] == 'X')                                        // if cmd is 'X', notify other threads to terminate
            {
                state->stopProducers = 1;
                pthread_cond_broadcast(&state->locks.producer_writeq_cond);
                break;
            }
            else if (cmdArr[numItems] >= 'A' && cmdArr[numItems] <= 'D')        // if cmd is valid, then add to temp queue
            {
                if (keyArr[numItems] >= 0 && keyArr[numItems] <= 1000)          // accepted range of value is [0-1000]
                    numItems++;
            }
        }

        pthread_mutex_unlock(&state->locks.producer_read_intput_mutex);         // release lock so that other thread can read from input

        for (int i = 0; i < numItems; i++)
        {
            tempItems[i].cmd = cmdArr[i];
            switch (cmdArr[i])                                                  // encrypt key by calling transform routines
            {
                case 'A': tempItems[i].key = transformA(keyArr[i]); break;
                case 'B': tempItems[i].key = transformB(keyArr[i]); break;
                case 'C': tempItems[i].key = transformC(keyArr[i]); break;
                case 'D': tempItems[i].key = transformD(keyArr[i]); break;
            }
        }
        addItemsToJobQueue(state, tempItems, numItems, threadId);
    }
    state->producers.count--;

    time_t endTime = time(NULL);                                                // update producer thread time
    pthread_mutex_lock(&state->locks.clock_mutex);
    state->producers.totalTime += difftime(endTime, startTime);
    pthread_mutex_unlock(&state->locks.clock_mutex);
}

// Consumer routine
void *consumer(void *arg)
{
    struct ThreadArg* threadArg = (struct ThreadArg *)arg;
    int threadId = threadArg->threadId;
    struct State *state = threadArg->state;
    state->consumers.count++;
    
    time_t startTime = time(NULL);                                                  //Get new time
    while (!(state->numItems == 0 && state->producers.count == 0))
    {
        uint16_t dkey[SIZE];
        struct work_entry tempQueue[SIZE];
        int numItems = -1;
        getItemsFromJobQueue(state, tempQueue, &numItems, threadId);                // get items from queue
        for (int i = 0; i < numItems; i++)
        {
            switch (tempQueue[i].cmd)                                               // decrypt key
            {
                case 'A': dkey[i] = transformA(tempQueue[i].key); break;
                case 'B': dkey[i] = transformB(tempQueue[i].key); break;
                case 'C': dkey[i] = transformC(tempQueue[i].key); break;
                case 'D': dkey[i] = transformD(tempQueue[i].key); break;
            }
        }

        pthread_mutex_lock(&state->locks.consumer_display_output_mutex);            // acquire lock to print to STDOUT
        
        while (state->consumers.pool[state->consumers.popItemIndex] != threadId)    // wait if it is some other threads trun to write to STDOUT
            pthread_cond_wait(&state->locks.consumer_display_cond, &state->locks.consumer_display_output_mutex);
        
        for (int i = 0; i < numItems; i++)                                          // print the output
            printf("Q:%d\t\t%c\t\t%hu\t\t%d\n", i, tempQueue[i].cmd, tempQueue[i].key, dkey[i]);
        
        state->consumers.pool[state->consumers.popItemIndex] = -1;                  // udpate thread id to keep track of next thread that should write to STDOUT
        state->consumers.popItemIndex = (state->consumers.popItemIndex + 1) % CONSUMER_THREAD_COUNT;

        pthread_cond_broadcast(&state->locks.consumer_display_cond);                // notify other consumers that it is safe to write to STDOUT
        pthread_mutex_unlock(&state->locks.consumer_display_output_mutex);
    }
    state->consumers.count--;

    time_t endTime = time(NULL);
    pthread_mutex_lock(&state->locks.clock_mutex);                                  // update consumer time
    state->consumers.totalTime += difftime(endTime, startTime);
    pthread_mutex_unlock(&state->locks.clock_mutex);
}

// routine to initialize initial state
void initializeState(struct State *state)
{
    state->stopProducers = -1;
    state->numItems = 0;
    state->producers.addItemIndex = 0;
    state->producers.popItemIndex = 0;
    state->producers.count = 0;
    state->consumers.addItemIndex = 0;
    state->consumers.popItemIndex = 0;
    state->consumers.count = 0;
    state->producers.totalTime = 0;
    state->consumers.totalTime = 0;

    // lock init
    pthread_mutex_init(&state->locks.modify_queue_mutex, NULL);
    pthread_mutex_init(&state->locks.producer_read_intput_mutex, NULL);
    pthread_mutex_init(&state->locks.consumer_display_output_mutex, NULL);
    pthread_mutex_init(&state->locks.clock_mutex, NULL);
    pthread_cond_init(&state->locks.consumer_readq_cond, NULL);
    pthread_cond_init(&state->locks.producer_writeq_cond, NULL);
    pthread_cond_init(&state->locks.consumer_display_cond, NULL);
}

// entry point
int main()
{
    struct State state;
    struct Locks locks;
    initializeState(&state);

    pthread_t producers[PRODUCER_THREAD_COUNT], consumers[CONSUMER_THREAD_COUNT];
    struct ThreadArg producerArgs[PRODUCER_THREAD_COUNT], consumerArgs[CONSUMER_THREAD_COUNT];

    for (int i = 0; i < PRODUCER_THREAD_COUNT; i++)                     // create producer threads
    {
        producerArgs[i].threadId = i;
        producerArgs[i].state = &state;
        if (pthread_create(&producers[i], NULL, producer, (void *)&producerArgs[i]))
            printf("failed to create theread\n");
    }

    for (int i = 0; i < CONSUMER_THREAD_COUNT; i++)                     // create consumer threads
    {
        consumerArgs[i].threadId = i;
        consumerArgs[i].state = &state;
        if (pthread_create(&consumers[i], NULL, consumer, (void *)&consumerArgs[i]))
            printf("failed to create theread\n");
    }

    for (int i = 0; i < PRODUCER_THREAD_COUNT; i++)                     // join producer threads
        pthread_join(producers[i], NULL);
    
    for (int i = 0; i < CONSUMER_THREAD_COUNT; i++)                     // join consumer threads
        pthread_join(consumers[i], NULL);
    
    printf("Total producer execution time using time(2) = %f\n", state.producers.totalTime);
    printf("Total consumer execution time using time(2) = %f\n", state.consumers.totalTime);
    
    // destroy locks
    pthread_mutex_destroy(&state.locks.modify_queue_mutex);
    pthread_mutex_destroy(&state.locks.producer_read_intput_mutex);
    pthread_mutex_destroy(&state.locks.consumer_display_output_mutex);
    pthread_mutex_destroy(&state.locks.clock_mutex);
    pthread_cond_destroy(&state.locks.producer_writeq_cond);
    pthread_cond_destroy(&state.locks.consumer_readq_cond);
    pthread_cond_destroy(&state.locks.consumer_display_cond);
    
    return 0;
}