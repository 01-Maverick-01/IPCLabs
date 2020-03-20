/****************************************************************************************/
/* CSE 5441 - Lab assignment 3                                                          */
/*      This program implements a multi threaded version of producer-consumer problem   */
/*      using OpenMP library. The producer is reposible for populating the queue and    */
/*      consumer reads this queue and displays output.                                  */
/*      There are in total 32 threads out of which first 16 are producers and rest are  */
/*      consumners.                                                                     */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Compile using 'icc' compiler                                                         */
/* Created on Mar 17 2020                                                               */
/*                                                                                      */
/****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <omp.h>

#define SIZE 5
#define PRODUCER_COUNT 16
#define CONSUMER_COUNT 15
#define OUTPUT_THREAD_COUNT 1
#define TOTAL_THREADS PRODUCER_COUNT+CONSUMER_COUNT+OUTPUT_THREAD_COUNT

uint16_t transformA(uint16_t input_val);
uint16_t transformB(uint16_t input_val);
uint16_t transformC(uint16_t input_val);
uint16_t transformD(uint16_t input_val);

struct work_entry
{
    char cmd;
    uint16_t key;
};

struct wrapper_work_entry
{
    struct work_entry values[SIZE];
    int readIndex;
};

struct output_queue
{
    struct output_queue *next;
    char output[100];
    int index;
};

struct State
{
    struct wrapper_work_entry items;
    struct output_queue *out_queue;
    int stopProducers;
    int count;
    int numProducersAlive;
    int numConsumersAlive;
    int currentReadIndex;
    int currentOutIndex;
    double producerTime;
    double consumerTime;
};

// thread safe method to add item to queue
void addItemsToJobQueue(struct State *state, struct wrapper_work_entry itemsToAdd, int numItemsToAdd)
{    
    #pragma omp critical (producer_buffer_access)
    {
        while (state->count > 0);

        #pragma omp critical (buffer_access)
        {
            printf("Producer thread %d writing to queue\n", omp_get_thread_num());
            state->items.readIndex = itemsToAdd.readIndex;
            for (int i = 0; i < numItemsToAdd; i++)                     // add items to queue
            {
                state->items.values[i].cmd = itemsToAdd.values[i].cmd;
                state->items.values[i].key = itemsToAdd.values[i].key;
                state->count++;
            }
        }
    }
}

// thread safe method to read items from queue
void getItemsFromJobQueue(struct State *state, struct wrapper_work_entry *itemsFound, int *numItemsFound)
{
    #pragma omp critical (buffer_access)
    {
        *numItemsFound = state->count;
        if (state->count > 0)
        {
            printf("consumer thread %d reading from queue\n", omp_get_thread_num());
            itemsFound->readIndex = state->items.readIndex;
            for (int i = 0; i < *numItemsFound; i++)                    // read items from queue
            {
                itemsFound->values[i].cmd = state->items.values[i].cmd;
                itemsFound->values[i].key = state->items.values[i].key;
                state->count--;
            }
        }
    }
}

// Producer routine
//      returns 0 if all cmd processed or if cmd 'X' is encountered, else returns 1
//      this routine also updates the 'numItems' variable to current queue count
void producer(struct State *state)
{
    // printf("Producer thread %d started\n", omp_get_thread_num());
    #pragma omp critical (stateAccess_producer)
    {
        state->numProducersAlive++;
    }
    
    time_t startTime = time(NULL);                                                  //Get new time
    while (!state->stopProducers)
    {
        char cmdArr[SIZE];
        uint16_t keyArr[SIZE];
        struct wrapper_work_entry tempItems;
        int numItems = 0;
        
        #pragma omp critical (std_input)
        {
            // printf("Producer thread %d reading input from STDIN\n", omp_get_thread_num());
            while(numItems < SIZE && !state->stopProducers)
            {
                scanf("%c  %hu\n", &cmdArr[numItems], &keyArr[numItems]);
                if (cmdArr[numItems] == 'X')                                        // if cmd is 'X', notify other threads to terminate
                {
                    // printf("Producer thread %d found X\n", omp_get_thread_num());
                    state->stopProducers = 1;
                    break;
                }
                else if (cmdArr[numItems] >= 'A' && cmdArr[numItems] <= 'D')        // if cmd is valid, then add to temp queue
                {
                    if (keyArr[numItems] >= 0 && keyArr[numItems] <= 1000)          // accepted range of value is [0-1000]
                        numItems++;
                }
            }
            tempItems.readIndex = ++state->currentReadIndex;
        }

        for (int i = 0; i < numItems; i++)
        {
            tempItems.values[i].cmd = cmdArr[i];
            switch (cmdArr[i])                                                      // encrypt key by calling transform routines
            {
                case 'A': tempItems.values[i].key = transformA(keyArr[i]); break;
                case 'B': tempItems.values[i].key = transformB(keyArr[i]); break;
                case 'C': tempItems.values[i].key = transformC(keyArr[i]); break;
                case 'D': tempItems.values[i].key = transformD(keyArr[i]); break;
            }
        }
        if (numItems > 0)
            addItemsToJobQueue(state, tempItems, numItems);
    }
    #pragma omp critical (stateAccess_producer)
    {
        state->producerTime += difftime(time(NULL), startTime);              // update producer thread time
        state->numProducersAlive--;
    }
    // printf("Producer thread %d terminated\n", omp_get_thread_num());
}

// Consumer routine
//      this routine also updates the 'numItems' variable to current queue count
void consumer(struct State *state)
{
    #pragma omp critical (consumer_thread_pool_update)
    {
        state->numConsumersAlive++;
    }

    time_t startTime = time(NULL);                                                  //Get new time
    // printf("Consumer thread %d started\n", omp_get_thread_num());
    while (!(state->count == 0 && state->numProducersAlive == 0))
    {
        // printf("Consumer thread %d running\n", omp_get_thread_num());
        uint16_t dkey;
        struct wrapper_work_entry tempQueue;
        int numItems = -1;
        getItemsFromJobQueue(state, &tempQueue, &numItems);                                // get items from queue
        if (numItems > 0)
        {
            char block_output[100];
            for (int i = 0; i < numItems; i++)
            {
                char output[25];
                switch (tempQueue.values[i].cmd)                                           // decrypt key
                {
                    case 'A': dkey = transformA(tempQueue.values[i].key); break;
                    case 'B': dkey = transformB(tempQueue.values[i].key); break;
                    case 'C': dkey = transformC(tempQueue.values[i].key); break;
                    case 'D': dkey = transformD(tempQueue.values[i].key); break;
                }
                sprintf(output, "Q:%d\t\t%c\t\t%hu\t\t%d\n", i, tempQueue.values[i].cmd, tempQueue.values[i].key, dkey);
                if (i == 0)
                    strcpy(block_output, output);
                else
                    strcat(block_output, output);
            }
            #pragma omp critical (state_output_queue)
            {
                printf("Consumer thread %d added to output queue\n", omp_get_thread_num());
                struct output_queue *out_queue = (struct output_queue *)malloc(sizeof(struct output_queue));
                strcpy(out_queue->output, block_output);
                out_queue->next = state->out_queue;
                out_queue->index = tempQueue.readIndex;
                state->out_queue = out_queue;
            }
        }
    }

    #pragma omp critical (consumer_thread_pool_update)
    {
        state->consumerTime += difftime(time(NULL), startTime);              // update producer thread time
        state->numConsumersAlive--;
    }
    // printf("comsumer thread %d terminated\n", omp_get_thread_num());
}

void outputHandler(struct State *state)
{
    while (!(state->numProducersAlive == 0 && state->numConsumersAlive == 0 && state->stopProducers) || state->out_queue != NULL)
    {
        struct output_queue *prev = NULL;
        struct output_queue *current = state->out_queue;
        while(current != NULL)
        {
            if (current->index == state->currentOutIndex)
            {
                printf("Output thread %d displayed item\n", omp_get_thread_num());
                //printf("%s", current->output);
                #pragma omp critical (state_output_queue)
                {
                    state->currentOutIndex++;
                    if (prev == NULL)
                        state->out_queue = current->next;
                    else
                        prev->next = current->next;
                    free(current);
                    current = NULL;
                }
            }
            else
            {
                prev = current;
                current = current->next;
            }
        }
    }
}

// routine to initialize initial state
void initializeState(struct State *state)
{
    state->stopProducers = 0;
    state->count = 0;
    state->currentOutIndex = 0;
    state->numProducersAlive = 0;
    state->currentReadIndex = -1;
    state->consumerTime = 0;
    state->producerTime = 0;
    state->out_queue = NULL;
}

// entry point
int main()
{
    struct State state;
    initializeState(&state);
    int thread_id;

    omp_set_num_threads(TOTAL_THREADS); //Set the number of threads
    #pragma omp parallel private(thread_id)
    {
        thread_id = omp_get_thread_num();
        if (omp_get_num_threads() == TOTAL_THREADS)
        {
            if (thread_id == 0)
                outputHandler(&state);
            else if (thread_id > 0 && thread_id <= PRODUCER_COUNT)
                producer(&state);
            else
                consumer(&state);
        }
        else
        {
            printf("Allocated thread count (%d) is less than the requested thread count (%d)\n", omp_get_num_threads(), TOTAL_THREADS);
            exit(0);
        }
    }

    printf("Total producer execution time using time(2) = %f\n", state.producerTime);
    printf("Total consumer execution time using time(2) = %f\n", state.consumerTime);

    return 0;
}
