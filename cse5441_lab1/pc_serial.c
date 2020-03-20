/****************************************************************************************/
/* CSE 5441 - Lab assignment 1                                                          */
/*      This program implements a single threaded version of producer-consumer problem. */
/*      The producer is reposible for populating the queue and once the queue is full,  */
/*      consumer reads this queue and displays some output. Once, all the items in the  */
/*      queue is processed by consumer, producer is called again. This continues until  */
/*      all items in the input file are processed or cmd 'X' is encountered as input.   */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Compile using 'icc' compiler                                                         */
/* Created on Thu Jan 30 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define SIZE 5

uint16_t transformA(uint16_t input_val);
uint16_t transformB(uint16_t input_val);
uint16_t transformC(uint16_t input_val);
uint16_t transformD(uint16_t input_val);

struct work_entry
{
    char cmd;
    uint16_t key;
};

double producer_time, consumer_time;            // global variables to store total time taken by producer and consumer

// Producer routine
//      returns 0 if all cmd processed or if cmd 'X' is encountered, else returns 1
//      this routine also updates the 'numItems' variable to current queue count
int producer(struct work_entry jobQueue[], int* numItems)
{
    time_t now1 = time(NULL);                   //Get current time in seconds
    char cmd;
    uint16_t key, ekey;
    while (*numItems < SIZE)
    {
        scanf("%c  %hu\n", &cmd, &key);         // read cmd and key from file using standard input
        if (key >= 0 && key <= 1000)            // accepted range of value is [0-1000]
        {
            switch (cmd)
            {
            case 'A': 
                jobQueue[*numItems].cmd = cmd;
                jobQueue[*numItems].key = transformA(key);            
                (*numItems)++;
                break;
            case 'B':  
                jobQueue[*numItems].cmd = cmd;
                jobQueue[*numItems].key = transformB(key);
                (*numItems)++;
                break;
            case 'C':  
                jobQueue[*numItems].cmd = cmd;
                jobQueue[*numItems].key = transformC(key);
                (*numItems)++;
                break;
            case 'D':  
                jobQueue[*numItems].cmd = cmd;
                jobQueue[*numItems].key = transformD(key);
                (*numItems)++;
                break;
            case 'X': return 0;
            default: break;                     // do nothing
            }
        }
    }

    time_t now2 = time(NULL);                   //Get new time
    producer_time += difftime(now2, now1);      //The difference between the two times is the time taken by producer()
    
    return 1;
}

// Consumer routine
//      this routine also updates the 'numItems' variable to current queue count
void consumer(struct work_entry jobQueue[], int* numItems)
{
    time_t now1 = time(NULL);                   //Get current time in seconds
    int currentItems = *numItems;
    uint16_t dkey;
    for (int i = 0; i < currentItems; i++)
    {
        switch (jobQueue[i].cmd)
        {
            case 'A': dkey = transformA(jobQueue[i].key); break;
            case 'B': dkey = transformB(jobQueue[i].key); break;
            case 'C': dkey = transformC(jobQueue[i].key); break;
            case 'D': dkey = transformD(jobQueue[i].key); break;
        }
        printf("Q:%d\t\t%c\t\t%hu\t\t%d\n", i, jobQueue[i].cmd, jobQueue[i].key, dkey);     // print the output
        (*numItems)--;
    }

    time_t now2 = time(NULL);                   //Get new time
    consumer_time += difftime(now2, now1);      //The difference between the two times is the time taken by producer()
}

// entry point
int main()
{
    int numItemsInQueue = 0;
    struct work_entry jobQueue[SIZE];

    while(producer(jobQueue, &numItemsInQueue) != 0)
        consumer(jobQueue, &numItemsInQueue);

    if (numItemsInQueue > 0)
        consumer(jobQueue, &numItemsInQueue);

    printf("Total producer execution time using time(2) = %f\n", producer_time);
    printf("Total consumer execution time using time(2) = %f\n", consumer_time);

    return 0;
}