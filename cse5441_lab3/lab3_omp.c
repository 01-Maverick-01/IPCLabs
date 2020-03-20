/****************************************************************************************/
/* CSE 5441 - Lab assignment 3                                                          */
/*      This program implements a multi threaded version of producer-consumer problem   */
/*      using OpenMP library. The producer is reposible for populating the queue and    */
/*      consumer reads this queue and displays output.                                  */
/*      There are in total 32 threads out of which 16 are producers and rest are        */
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
#define CONSUMER_COUNT 16
#define TOTAL_THREADS PRODUCER_COUNT+CONSUMER_COUNT

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

// struct for output queue (linked list)
struct output_queue
{
    struct output_queue *next;
    char output[100];
    int index;
};

// struct to represent program state
struct State
{
    struct wrapper_work_entry items;        // common queue for producer/consumer
    struct output_queue *out_queue;         // output queue
    int stopProducers;                      // flag to stop producers after 'X' is encountered
    int count;                              // number of items in common queue
    int numProducersAlive;                  // number of producers threads currently alive
    int numConsumersAlive;                  // number of consumer threads currently alive
    int currentReadIndex;                   // input order for items
    int currentOutIndex;                    // current index order for output
    double producerTime;                    // total time taken by producers
    double consumerTime;                    // total time taken by consumers
};

// thread safe method to add item to queue
void addItemsToJobQueue(struct State *state, struct wrapper_work_entry itemsToAdd, int numItemsToAdd)
{    
    #pragma omp critical (producer_buffer_access)                       // section to prevent second producer
    {
        while (state->count > 0);                                       // wait if queue is full

        #pragma omp critical (buffer_access)                            // section to prevent prducer/consumer simultaneous access to queue
        {
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
    #pragma omp critical (buffer_access)                                // section to prevent prducer/consumer simultaneous access to queue
    {
        *numItemsFound = state->count;
        if (state->count > 0)
        {
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

// thread safe method to add items to output queue
void addToDisplayQueue(struct State *state, char *block_output, struct wrapper_work_entry tempQueue)
{
    #pragma omp critical (state_output_queue)
    {
        struct output_queue *out_queue = (struct output_queue *)malloc(sizeof(struct output_queue));
        strcpy(out_queue->output, block_output);
        out_queue->next = state->out_queue;
        out_queue->index = tempQueue.readIndex;
        state->out_queue = out_queue;
    }
}

// Producer routine
void producer(struct State *state)
{
    #pragma omp critical (stateAccess_producer)                                 // critical section to update producer details
    {
        state->numProducersAlive++;
    }
    
    time_t startTime = time(NULL);                                              //Get new time
    while (!state->stopProducers)
    {
        char cmdArr[SIZE];
        uint16_t keyArr[SIZE];
        struct wrapper_work_entry tempItems;
        int numItems = 0;
        
        #pragma omp critical (std_input)                                        // critical section to read input from STDIN
        {
            while(numItems < SIZE && !state->stopProducers)
            {
                scanf("%c  %hu\n", &cmdArr[numItems], &keyArr[numItems]);
                if (cmdArr[numItems] == 'X')                                    // if cmd is 'X', notify other threads to terminate
                {
                    state->stopProducers = 1;
                    break;
                }
                else if (cmdArr[numItems] >= 'A' && cmdArr[numItems] <= 'D')    // if cmd is valid, then add to temp queue
                {
                    if (keyArr[numItems] >= 0 && keyArr[numItems] <= 1000)      // accepted range of value is [0-1000]
                        numItems++;
                }
            }
            tempItems.readIndex = ++state->currentReadIndex;
        }

        for (int i = 0; i < numItems; i++)
        {
            tempItems.values[i].cmd = cmdArr[i];
            switch (cmdArr[i])                                                  // encrypt key by calling transform routines
            {
                case 'A': tempItems.values[i].key = transformA(keyArr[i]); break;
                case 'B': tempItems.values[i].key = transformB(keyArr[i]); break;
                case 'C': tempItems.values[i].key = transformC(keyArr[i]); break;
                case 'D': tempItems.values[i].key = transformD(keyArr[i]); break;
            }
        }
        if (numItems > 0)                                                       // add items to common queue
            addItemsToJobQueue(state, tempItems, numItems);
    }
    #pragma omp critical (stateAccess_producer)                                 // critical section to update producer details
    {
        state->producerTime += difftime(time(NULL), startTime);                 // update producer thread time
        state->numProducersAlive--;
    }
    // printf("Producer thread %d terminated\n", omp_get_thread_num());
}

// Consumer routine
void consumer(struct State *state)
{
    #pragma omp critical (consumer_thread_pool_update)                              // critical section to update consumer time
    {
        state->numConsumersAlive++;
    }

    time_t startTime = time(NULL);                                                  //Get new time
    while (!(state->count == 0 && state->numProducersAlive == 0))
    {
        uint16_t dkey;
        struct wrapper_work_entry tempQueue;
        int numItems = -1;
        getItemsFromJobQueue(state, &tempQueue, &numItems);                         // get items from queue
        if (numItems > 0)
        {
            char block_output[100];
            for (int i = 0; i < numItems; i++)
            {
                char output[25];
                switch (tempQueue.values[i].cmd)                                    // decrypt key
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
            addToDisplayQueue(state, block_output, tempQueue);                      // add items to display queue
        }
    }

    #pragma omp critical (consumer_thread_pool_update)                              // critical section to update consumer time
    {
        state->consumerTime += difftime(time(NULL), startTime);                     // update producer thread time
        state->numConsumersAlive--;
    }
}

// method to display output on STDOUT
void displayOutput(struct State *state)
{
    while (state->out_queue != NULL)                                                // traverse until all items are displayed
    {
        struct output_queue *prev = NULL;
        struct output_queue *current = state->out_queue;
        while(current != NULL)
        {
            if (current->index == state->currentOutIndex)                           // display items in order the order in which they were read
            {
                printf("%s", current->output);
                state->currentOutIndex++;
                if (prev == NULL)
                    state->out_queue = current->next;
                else
                    prev->next = current->next;
                free(current);                                                      // delete displayed item
                current = NULL;
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

    #pragma omp parallel private(thread_id) shared(state) num_threads(TOTAL_THREADS)
    {
        thread_id = omp_get_thread_num();
        if (omp_get_num_threads() == TOTAL_THREADS)
        {
            if (thread_id %2 == 0)
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

    displayOutput(&state);    
    printf("Total producer execution time using time(2) = %f\n", state.producerTime);
    printf("Total consumer execution time using time(2) = %f\n", state.consumerTime);

    return 0;
}
