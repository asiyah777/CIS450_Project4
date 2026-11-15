#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

// Define crossing-policy constants
#define NORTH 0
#define SOUTH 1
#define GREEDY 0
#define FCFS 1
#define MAX_ON_BRIDGE 3
#define CROSSING_TIME 5.0

// Vehicle information structure taken and modified from assignment instructions
typedef struct _vehicle_info {
    int id; // vehicle ID
    int dir; // heading direction (N or S)
    double inter_arrival_t; // inter-arrival time before next vehicle
    semaphore_t permission; // ADDED: If vehicle has permission to cross
    struct _vehicle_info *next; // ADDED: Next vehicle in respective queue
} vehicle_info;

// FIFO queue for waiting cars
typedef struct _vehicle_queue {
    vehicle_info *head;
    vehicle_info *tail;
    int count;
} vehicle_queue;

// Shared key resouces: direction of traffic on the brdige, vehicles on the bridge, and waiting queues
static int bridge_direction = -1; // Direction of vehicles currently on the bridge (-1 for none, 0 for north, 1 for south)
static int bridge_count = 0; // Number of vehicles currently on the bridge
static semaphore_t bridgeMutex;
static vehicle_queue waitingQueues[2];
static int crossing_policy = GREEDY;

static const char *DirectionName(int dir)
{
    return dir == NORTH ? "North" : "South";
}

static void EnqueueVehicle(vehicle_queue *queue, vehicle_info *vehicle)
{
    vehicle->next = NULL;

    if (queue->tail == NULL) {
        queue->head = vehicle;
        queue->tail = vehicle;
    } else {
        queue->tail->next = vehicle;
        queue->tail = vehicle;
    }

    queue->count++;
}

static vehicle_info *DequeueVehicle(vehicle_queue *queue)
{
    vehicle_info *vehicle = queue->head;

    if (vehicle != NULL) {
        queue->head = vehicle->next;

        if (queue->head == NULL) {
            queue->tail = NULL;
        }

        vehicle->next = NULL;
        queue->count--;
    }

    return vehicle;
}

// TO DO:
void ArriveBridge(vehicle_info *ti)
{
    Sem_Wait(&bridgeMutex);
    Sem_Post(&bridgeMutex);
    Sem_Wait(&ti->permission);
}


// TO DO:
void CrossBridge(vehicle_info *ti)
{
    double startTime = ElapsedTime();

    printf("Time %.1f: Vehicle %d (%s) crossing\n",
           startTime, ti->id, DirectionName(ti->dir));
    fflush(stdout);

    Spin(CROSSING_TIME);
}

// TO DO:
void ExitBridge(vehicle_info *ti)
{
    Sem_Wait(&bridgeMutex);

    bridge_count--;

    if (bridge_count == 0) {
        bridge_direction = -1;
    }

    Sem_Post(&bridgeMutex);
}

void *VehicleAction(void *arg)
{
    vehicle_info *ti = (vehicle_info *) arg;

    ArriveBridge(ti);
    CrossBridge(ti);
    ExitBridge(ti);
    return NULL;
}

int main(int argc, char *argv[])
{
    vehicle_info vehicles[13] = {
        { .id = 0, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 1, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 2, .dir = SOUTH, .inter_arrival_t = 1.0 },
        { .id = 3, .dir = SOUTH, .inter_arrival_t = 1.0 },
        { .id = 4, .dir = SOUTH, .inter_arrival_t = 1.0 },
        { .id = 5, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 6, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 7, .dir = SOUTH, .inter_arrival_t = 1.0 },
        { .id = 8, .dir = SOUTH, .inter_arrival_t = 1.0 },
        { .id = 9, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 10, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 11, .dir = NORTH, .inter_arrival_t = 1.0 },
        { .id = 12, .dir = NORTH, .inter_arrival_t = 0.0 }
    };

    const int numVehicles = (int)(sizeof(vehicles) / sizeof(vehicles[0]));
    thread_t threads[sizeof(vehicles) / sizeof(vehicles[0])];
    int i;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <crossing_policy>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "greedy") == 0) {
        printf("Using greedy crossing policy\n");
        crossing_policy = GREEDY;
    } else if (strcmp(argv[1], "fcfs") == 0) {
        printf("Using FCFS crossing policy\n");
        crossing_policy = FCFS;
    } else {
        fprintf(stderr, "Invalid crossing policy: %s\n", argv[1]);
        exit(1);
    }

    // Initialize shared mutex and set each vehicle's permission to 0
    Sem_Init(&bridgeMutex, 1);
    for (i = 0; i < numVehicles; i++) {
        Sem_Init(&vehicles[i].permission, 0);
    }

    gStartTime = GetTime();

    /* Create the threads, sleeping between arrivals */
    for (i = 0; i < numVehicles; i++) {
        Thread_Create(&threads[i], VehicleAction, &vehicles[i]);
        SleepSeconds(vehicles[i].inter_arrival_t);
    }

    /* Wait for every thread to finish */
    for (i = 0; i < numVehicles; i++) {
        Thread_Join(&threads[i]);
    }

    Sem_Destroy(&bridgeMutex);
    return 0;
}