/****************************************************************************************/
/* CSE 5441 - Lab assignment 4                                                          */
/*      This program implements a multi threaded version of producer-consumer problem   */
/*      using MPI. The code is going to user 5 MPI nodes and node with Rank 0 will be   */
/*      the master MPI node and Rank 1-4 will perform parallel transform computations.  */
/*      Every slave node will be running 1 producer and 1 Consumer threads which will   */
/*      processing items in the nodes job queue and returning results back to master.   */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Compile using 'mpicc' compiler                                                       */
/* Created on Apr 03 2020                                                               */
/*                                                                                      */
/****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <omp.h>
#include <mpi.h>

#define NUM_NODES 5                                             // total number of nodes
#define LEADER 0                                                // rank of leader
#define SIZE 5                                                  // common queue size
#define PRODUCER_COUNT 4                                        // number of producer threads per slave node
#define CONSUMER_COUNT 4                                        // number of consumer threads per slave node
#define TOTAL_THREADS PRODUCER_COUNT+CONSUMER_COUNT+1           // total number of threads per slave node (P+C+leader_thread)
// Messages
#define MSG_TERMINATE 0                                         // terminate message
#define MSG_SENDING_DATA 1                                      // data transfer message
// Message Type
#define MSG_TYPE_DATA 0                                         // tag value for data tranfer messages
#define MSG_TYPE_CMD 1                                          // tag value for cmd messages
#define MSG_TYPE_TIME_DATA 2                                    // tag value for time data tranfer messages
// Message indexes
#define INDEX_LINE_NUMBER 0                                     // line number index in data array trasferred between leader and slave
#define INDEX_CMD 1                                             // input cmd(A,B,C,D) index in data array trasferred between leader and slave
#define INDEX_KEY 2                                             // input key index in data array trasferred between leader and slave
#define INDEX_EKEY 3                                            // encrypted key index in data array trasferred between leader and slave
#define INDEX_DKEY 4                                            // decrypted key index in data array trasferred between leader and slave
#define INDEX_P_RANK 5                                          // node rank (one that encrypted cmd-key pair) index in data array
#define INDEX_P_TID 6                                           // thread id (one that encrypted cmd-key pair) index in data array
#define INDEX_C_RANK 7                                          // node rank (one that decrypted cmd-key pair) index in data array
#define INDEX_C_TID 8                                           // thread id (one that decrypted cmd-key pair) index in data array
#define INDEX_P_TIME 0                                          // total producer time (taken on a node) index in time data array
#define INDEX_C_TIME 1                                          // total consumer time (taken on a node) index in time data array

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
    struct work_entry value;                                    // work entry
    int eKey;                                                   // encrypted key
    int dKey;                                                   // decrypted key
    int lineNumber;                                             // index on input file (excluding invalid entries)
    int producerRank;                                           // rank of node that encrypted data
    int producerThreadId;                                       // thread id of thread that encrypted data
    int consumerRank;                                           // rank of node that decrypted data
    int consumerThreadId;                                       // thread id of node that decrypted data
};

struct output_queue                                             // struct for output queue of leader node (linked list)
{
    struct output_queue *next;
    struct wrapper_work_entry data;
};

struct node_job_queue                                           // struct for job queue for slave nodes (linked list)
{
    struct node_job_queue *next;
    struct wrapper_work_entry data;
};

// struct to represent program state on a slave node
struct SlaveNodeState
{
    struct node_job_queue *jobQueue;                            // job queue for a slave node
    struct wrapper_work_entry commonQueue[SIZE];                // common queue for producers and consumers
    int stopProducers;                                          // flag to stop producers after 'X' is encountered
    int common_queue_count;                                     // number of items in common queue
    int numProducersAlive;                                      // number of producers threads currently alive
    int numConsumersAlive;                                      // number of consumer threads currently alive
    double producerTime;                                        // total time taken by producers
    double consumerTime;                                        // total time taken by consumers
    int rank;                                                   // node rank
};

/******************************************** MPI Related Send/Recv methods ********************************************/

// This method sends terminate messages to all slave nodes.
void sendTerminateToAllSlaves()
{
    int msg = MSG_TERMINATE;
    for (int i = 1; i < NUM_NODES; i++)
        MPI_Send(&msg, 1, MPI_INT, i, MSG_TYPE_CMD, MPI_COMM_WORLD);
}

// This method sends data from slave node to leader node.
void sendMsgToLeaderNode(int *msg, int size)
{
    MPI_Send(msg, size, MPI_INT, LEADER, MSG_TYPE_DATA, MPI_COMM_WORLD);
}

// This method sends jobs from leader node to slave node.
void sendJobToSlaveNode(int index, char cmd, int key, int pid)
{
    int msgCmd = MSG_SENDING_DATA;
    int msg[3] = { index, cmd, key };
    MPI_Send(&msgCmd, 1, MPI_INT, pid, MSG_TYPE_CMD, MPI_COMM_WORLD);
    MPI_Send(msg, 3, MPI_INT, pid, MSG_TYPE_DATA, MPI_COMM_WORLD);
}

// This method receives command from leader nodes. This is called by leader thread on the slave nodes.
int recvCmdFromLeader()
{
    int cmd;
    MPI_Status status;
    MPI_Recv(&cmd, 1, MPI_INT, 0, MSG_TYPE_CMD, MPI_COMM_WORLD, &status);
    return cmd;
}

// This method receives job from leader nodes. This is called by leader thread on the slave nodes.
void recvJobFromLeader(struct wrapper_work_entry *job)
{
    int data[3];
    MPI_Status status;
    MPI_Recv(data, 3, MPI_INT, 0, MSG_TYPE_DATA, MPI_COMM_WORLD, &status);
    job->value.cmd = data[INDEX_CMD];
    job->value.key = data[INDEX_KEY];
    job->lineNumber = data[INDEX_LINE_NUMBER];
}

// This method checks if there is any recv pending on leader node.
int isAnyReadPendingInLeaderBuffer()
{
    int flag;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, MSG_TYPE_DATA, MPI_COMM_WORLD, &flag, &status);
    return flag;
}

// This method receives data from any of the slave nodes. It is called by leader node.
void recvDataFromAnySlave(struct wrapper_work_entry *data)
{
    int recvdData[9];
    MPI_Status status;
    MPI_Recv(recvdData, 9, MPI_INT, MPI_ANY_SOURCE, MSG_TYPE_DATA, MPI_COMM_WORLD, &status);
    data->value.cmd = recvdData[INDEX_CMD];
    data->value.key = recvdData[INDEX_KEY];
    data->lineNumber = recvdData[INDEX_LINE_NUMBER];
    data->consumerRank = recvdData[INDEX_C_RANK];
    data->consumerThreadId = recvdData[INDEX_C_TID];
    data->producerRank = recvdData[INDEX_P_RANK];
    data->producerThreadId = recvdData[INDEX_P_TID];
    data->eKey = recvdData[INDEX_EKEY];
    data->dKey = recvdData[INDEX_DKEY];
}

// This method receives and updates the total producer and consumer elaspsed time.
void getElapsedTimeFromSlaves(double *producerTime, double *consumerTime)
{
    (*producerTime) = 0;
    (*consumerTime) = 0;
    for (int i = 1; i < NUM_NODES; i++)
    {
        double timeArr[2];
        MPI_Status status;
        MPI_Recv(timeArr, 2, MPI_DOUBLE, i, MSG_TYPE_TIME_DATA, MPI_COMM_WORLD, &status);
        (*producerTime) = (*producerTime) + timeArr[INDEX_P_TIME];
        (*consumerTime) = (*consumerTime) + timeArr[INDEX_C_TIME];
    }
}

/***********************************************************************************************************************/

void sendDataToLeaderNode(struct wrapper_work_entry tempQueue[SIZE], int numItems)
{
    #pragma omp critical (sending_to_leader_node)                // critical section to ensure only one thread sends msg to leader node.
    {
        for (int i = 0; i < numItems; i++)
        {
            int msg[9];
            msg[INDEX_C_RANK] = tempQueue[i].consumerRank;
            msg[INDEX_C_TID] = tempQueue[i].consumerThreadId;
            msg[INDEX_CMD] = tempQueue[i].value.cmd;
            msg[INDEX_KEY] = tempQueue[i].value.key;
            msg[INDEX_LINE_NUMBER] = tempQueue[i].lineNumber;
            msg[INDEX_EKEY] = tempQueue[i].eKey;
            msg[INDEX_DKEY] = tempQueue[i].dKey;
            msg[INDEX_P_RANK] = tempQueue[i].producerRank;
            msg[INDEX_P_TID] = tempQueue[i].producerThreadId;
            sendMsgToLeaderNode(msg, 9);
        }
    }
}

// thread safe method to add item to common queue
void addItemsToJobQueue(struct SlaveNodeState *state, struct wrapper_work_entry itemsToAdd[], int numItemsToAdd)
{    
    #pragma omp critical (producer_buffer_access)                       // section to prevent second producer
    {
        while (state->common_queue_count > 0);                          // wait if queue is full

        #pragma omp critical (buffer_access)                            // section to prevent prducer/consumer simultaneous access to queue
        {
            memcpy(&state->commonQueue, itemsToAdd, sizeof(struct wrapper_work_entry)* numItemsToAdd);
            state->common_queue_count = numItemsToAdd;
        }
    }
}

// thread safe method to read items from common queue
int getItemsFromJobQueue(struct SlaveNodeState *state, struct wrapper_work_entry *itemsFound)
{
    int numItemsFound = 0;
    #pragma omp critical (buffer_access)                               // section to prevent prducer/consumer simultaneous access to queue
    {
        numItemsFound = state->common_queue_count;                     // read items from queue
        if (state->common_queue_count > 0)
        {
            memcpy(itemsFound, &state->commonQueue, sizeof(struct wrapper_work_entry)*state->common_queue_count);
            state->common_queue_count = 0;
        }
    }
    return numItemsFound;
}

// thread safe method to read items from nodes job queue
int getJobsFromSlaveNodeQueue(struct SlaveNodeState *state, struct wrapper_work_entry *itemsFound)
{
    int numItemsFound = 0;
    #pragma omp critical (slave_node_queue_access)                         // section to prevent leader thread and other thread
    {                                                                      // simultaneous access to queue
        struct node_job_queue *temp = state->jobQueue;
        while(temp != NULL && numItemsFound != 5)
        {
            memcpy(&itemsFound[numItemsFound], &temp->data, sizeof(struct wrapper_work_entry));
            numItemsFound++;
            state->jobQueue = temp->next;
            free(temp);
            temp = state->jobQueue;
        }
    }
    return numItemsFound;
}

// this method adds job to slave node queue
void addJobToSlaveNodeQueue(struct SlaveNodeState *state, struct wrapper_work_entry job)
{
    #pragma omp critical (slave_node_queue_access) 
    {
        struct node_job_queue *tempJob = (struct node_job_queue *)malloc(sizeof(struct node_job_queue));
        memcpy(&tempJob->data, &job, sizeof(struct wrapper_work_entry));
        tempJob->next = state->jobQueue;
        state->jobQueue = tempJob;
    }
}

// this method is called by leader node to display the final output serially.
void displayOutput(struct output_queue *outputQueue)
{
    int currentIndex = 0;
    while (outputQueue != NULL)                                                // traverse until all items are displayed
    {
        struct output_queue *prev = NULL;
        struct output_queue *current = outputQueue;
        while(current != NULL)
        {
            if (current->data.lineNumber == currentIndex)                      // display items in order the order in which they were read
            {
                printf("Q:%d\t\t%c\t\t%hu\t\t%d\n", current->data.lineNumber%5, current->data.value.cmd, current->data.eKey, current->data.dKey);
                currentIndex++;
                if (prev == NULL)
                    outputQueue = current->next;
                else
                    prev->next = current->next;
                free(current);                                                 // delete displayed item
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

// Producer routine
void producer(struct SlaveNodeState *state, int rank)
{
    #pragma omp critical (stateAccess_producer)                                 // critical section to update producer details
    {
        state->numProducersAlive++;
    }
    
    time_t startTime = time(NULL);                                              //Get new time
    while (!(state->stopProducers && state->jobQueue == NULL))
    {
        struct wrapper_work_entry tempItems[SIZE];
        int numItems = getJobsFromSlaveNodeQueue(state, tempItems);        

        for (int i = 0; i < numItems; i++)
        {
            switch (tempItems[i].value.cmd)                                     // encrypt key by calling transform routines
            {
                case 'A': tempItems[i].eKey = transformA(tempItems[i].value.key); break;
                case 'B': tempItems[i].eKey = transformB(tempItems[i].value.key); break;
                case 'C': tempItems[i].eKey = transformC(tempItems[i].value.key); break;
                case 'D': tempItems[i].eKey = transformD(tempItems[i].value.key); break;
            }
            tempItems[i].producerRank = rank;
            tempItems[i].producerThreadId = omp_get_thread_num();
        }
        if (numItems > 0)                                                       // add items to common queue
            addItemsToJobQueue(state, tempItems, numItems);
    }
    #pragma omp critical (stateAccess_producer)                                 // critical section to update producer details
    {
        state->producerTime += difftime(time(NULL), startTime);                 // update producer thread time
        state->numProducersAlive--;
    }
}

// Consumer routine
void consumer(struct SlaveNodeState *state, int rank)
{
    #pragma omp critical (consumer_thread_pool_update)                              // critical section to update consumer time
    {
        state->numConsumersAlive++;
    }

    time_t startTime = time(NULL);                                                  //Get new time
    while (!(state->common_queue_count == 0 && state->numProducersAlive == 0))
    {
        uint16_t dkey;
        struct wrapper_work_entry tempQueue[SIZE];
        int numItems = getItemsFromJobQueue(state, tempQueue);                      // get items from queue
        if (numItems > 0)
        {
            char block_output[100];
            for (int i = 0; i < numItems; i++)
            {
                char output[25];
                switch (tempQueue[i].value.cmd)                                     // decrypt key
                {
                    case 'A': tempQueue[i].dKey = transformA(tempQueue[i].eKey); break;
                    case 'B': tempQueue[i].dKey = transformB(tempQueue[i].eKey); break;
                    case 'C': tempQueue[i].dKey = transformC(tempQueue[i].eKey); break;
                    case 'D': tempQueue[i].dKey = transformD(tempQueue[i].eKey); break;
                }
                tempQueue[i].consumerRank = rank;
                tempQueue[i].consumerThreadId = omp_get_thread_num();
            }
            sendDataToLeaderNode(tempQueue, numItems);                              // add items to display queue
        }
    }

    #pragma omp critical (consumer_thread_pool_update)                             // critical section to update consumer time
    {
        state->consumerTime += difftime(time(NULL), startTime);                    // update producer thread time
        state->numConsumersAlive--;
    }
}

// routine to initialize initial slave node state
void initializeSlaveState(struct SlaveNodeState *state)
{
    state->common_queue_count = 0;
    state->numConsumersAlive = 0;
    state->numProducersAlive = 0;
    state->stopProducers = 0;
    state->jobQueue = NULL;
}

// routine to check if input is valid.
//      -> This method return file pointer if input is valid.
FILE *isInputValid(int argc, char* argv[])
{
    FILE *fp = NULL;
    if (argc != 2)
        printf("Invalid number of arguments specified\n");
    else
    {
        fp = fopen(argv[1], "r");
        if (fp == NULL)
            printf("Specified file '%s' not found.\n", argv[1]);
    }
    
    return fp;
}

// work done by leader node
void startLeaderNode(int argc, char *argv[])
{
    FILE *fp = isInputValid(argc, argv);
    struct output_queue *outputQueue = NULL;
    if (fp != NULL)
    {
        char cmd;
        uint16_t key;
        int numItems = 0, pid = 1;
        do 
        {
            fscanf(fp, "%c  %hu\n", &cmd, &key);
            if (cmd >= 'A' && cmd <= 'D')                               // if cmd is valid, then add to temp queue
            {
                if (key >= 0 && key <= 1000)                            // accepted range of value is [0-1000]
                {
                    sendJobToSlaveNode(numItems, cmd, key, pid);
                    pid++;
                    numItems++;
                    if (pid == NUM_NODES)
                        pid = 1;
                }
            }
            
            while (isAnyReadPendingInLeaderBuffer())
            {
                struct wrapper_work_entry item;
                recvDataFromAnySlave(&item);
                struct output_queue *temp = (struct output_queue *)malloc(sizeof(struct output_queue));
                memcpy(&temp->data, &item, sizeof(struct wrapper_work_entry));
                temp->next = outputQueue;
                outputQueue = temp;
            }
        } while (cmd != 'X');
    }
    sendTerminateToAllSlaves();
    fclose(fp);
    
    MPI_Barrier(MPI_COMM_WORLD);
    while (isAnyReadPendingInLeaderBuffer())
    {
        struct wrapper_work_entry item;
        recvDataFromAnySlave(&item);
        struct output_queue *temp = (struct output_queue *)malloc(sizeof(struct output_queue));
        memcpy(&temp->data, &item, sizeof(struct wrapper_work_entry));
        temp->next = outputQueue;
        outputQueue = temp;
    }
    displayOutput(outputQueue);
    double producerTime = 0, consumerTime = 0;
    getElapsedTimeFromSlaves(&producerTime, &consumerTime);
    
    printf("Total producer execution time using time(2) = %f\n", producerTime);
    printf("Total consumer execution time using time(2) = %f\n", consumerTime);
}

// work done by slave nodes
void startSlaveNode(int rank)
{
    struct SlaveNodeState state;
    initializeSlaveState(&state);
    state.rank = rank;
    int thread_id;
    #pragma omp parallel private(thread_id) shared(state) num_threads(TOTAL_THREADS)
    {
        thread_id = omp_get_thread_num();
        if (omp_get_num_threads() == TOTAL_THREADS)
        {
            if (thread_id == LEADER)
            {
                int cmd;
                do
                {
                    cmd = recvCmdFromLeader();
                    if (cmd == MSG_SENDING_DATA)
                    {
                        struct wrapper_work_entry job;
                        recvJobFromLeader(&job);
                        addJobToSlaveNodeQueue(&state, job);
                    }
                    else if (cmd == MSG_TERMINATE)
                        state.stopProducers = 1;
                } while (cmd != MSG_TERMINATE);
            }
            else
            {
                if (thread_id %2 == 0)
                    producer(&state, rank);
                else
                    consumer(&state, rank);
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double timeArr[2] = { state.producerTime, state.consumerTime };
    MPI_Send(timeArr, 2, MPI_DOUBLE, LEADER, MSG_TYPE_TIME_DATA, MPI_COMM_WORLD);
}

// entry point
int main(int argc, char *argv[])
{
    int rank;
    MPI_Status stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == LEADER)
        startLeaderNode(argc, argv);
    else
        startSlaveNode(rank);
    
    MPI_Finalize();
    return 0;
}