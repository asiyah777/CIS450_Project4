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
    unsigned long arrival_order; // ADDED: Order of arrival for FCFS policy
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
// Shared variables for synchronization
static semaphore_t bridgeMutex;
static vehicle_queue waitingQueues[2];
static int crossing_policy = GREEDY;

// Arrivals also trigger FCFS admission while holding bridgeMutex.
static void WakeUpFCFS(void);

// Checks if a vehicle can enter using the Greedy policy.
// Up to 3 vehicles can cross if they are going the same direction.
int CanEnterGreedy(int direction)
{
    // If the bridge is empty, the vehicle can enter.
    if (bridge_count == 0) {
        return 1;
    }

    // Allow another vehicle if there is space and directions match.
    if (bridge_count < MAX_ON_BRIDGE && bridge_direction == direction) {
        return 1;
    }

    // Otherwise, the vehicle must wait.
    return 0;
}


// Return the one-letter direction used in the assignment's expected output.
static char DirectionName(int dir)
{
    return dir == NORTH ? 'N' : 'S';
}

static void EnqueueVehicle(vehicle_queue *queue, vehicle_info *vehicle)
{
    vehicle->next = NULL;

    // FCFS keeps vehicles in their assigned arrival order.
    if (crossing_policy == FCFS) {
        vehicle_info *current = queue->head;
        vehicle_info *previous = NULL;

        while (current != NULL &&
               current->arrival_order < vehicle->arrival_order) {
            previous = current;
            current = current->next;
        }

        if (previous == NULL) {
            queue->head = vehicle;
        } else {
            previous->next = vehicle;
        }

        vehicle->next = current;

        if (current == NULL) {
            queue->tail = vehicle;
        }
    } else {
        // Greedy keeps the original FIFO queue.
        if (queue->tail == NULL) {
            queue->head = vehicle;
            queue->tail = vehicle;
        } else {
            queue->tail->next = vehicle;
            queue->tail = vehicle;
        }
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


void ArriveBridge(vehicle_info *ti)
{
    Sem_Wait(&bridgeMutex);

    // Greedy policy: let the vehicle enter if the bridge has room and all vehicles are traveling in the same direction.
    if (crossing_policy == GREEDY && CanEnterGreedy(ti->dir)) {
        bridge_count++;
        bridge_direction = ti->dir;

        // Give this vehicle permission to cross.
        Sem_Post(&ti->permission);
    } else {
        // FCFS policy: enqueue the vehicle in the appropriate waiting queue.
        EnqueueVehicle(&waitingQueues[ti->dir], ti);
        if (crossing_policy == FCFS) {
            WakeUpFCFS();
        }
    }

    Sem_Post(&bridgeMutex);

    // Wait until this vehicle has permission to cross.
    Sem_Wait(&ti->permission);
}


 // Gives waiting vehicles permission to cross using Greedy.
 // This function must be called while bridgeMutex is locked.
void WakeUpGreedy(void)
 {
     int direction;

     
if (bridge_count == 0) {
    // If nobody is waiting, there is nothing to do.
    if (waitingQueues[NORTH].count == 0 &&
        waitingQueues[SOUTH].count == 0) {
        return;
    }

    // If both directions have waiting vehicles, choose
    // the direction of the vehicle that arrived first.
    if (waitingQueues[NORTH].count > 0 &&
        waitingQueues[SOUTH].count > 0) {
        if (waitingQueues[NORTH].head->id <
            waitingQueues[SOUTH].head->id) {
            direction = NORTH;
        } else {
            direction = SOUTH;
        }
    } else if (waitingQueues[NORTH].count > 0) {
        direction = NORTH;
    } else {
        direction = SOUTH;
    }


     } else {
         // If vehicles are crossing, only allow the same direction.
         direction = bridge_direction;
     }

     // Let waiting vehicles enter until the bridge is full.
     while (waitingQueues[direction].count > 0 &&
            CanEnterGreedy(direction)) {

         vehicle_info *nextVehicle =
             DequeueVehicle(&waitingQueues[direction]);

         bridge_direction = direction;
         bridge_count++;

         // Wake up this vehicle so it can start crossing.
         Sem_Post(&nextVehicle->permission);
     }
 }



// FCFS admits vehicles in arrival order.
// This function must be called while bridgeMutex is locked.
static void WakeUpFCFS(void)
{
    int direction;

    // If the bridge is empty, the oldest waiting vehicle chooses the direction.
    if (bridge_count == 0) {
        if (waitingQueues[NORTH].count == 0 &&
            waitingQueues[SOUTH].count == 0) {
            return;
        }

        if (waitingQueues[NORTH].count == 0) {
            direction = SOUTH;
        } else if (waitingQueues[SOUTH].count == 0) {
            direction = NORTH;
        } else {
            direction =
                waitingQueues[NORTH].head->arrival_order <
                waitingQueues[SOUTH].head->arrival_order
                ? NORTH : SOUTH;
        }
    } else {
        // Vehicles already on the bridge determine its current direction.
        direction = bridge_direction;
    }

    // Admit vehicles while there is room and FCFS order is not violated.
    while (waitingQueues[direction].count > 0 &&
           CanEnterGreedy(direction)) {

        vehicle_info *nextVehicle = waitingQueues[direction].head;

        // Do not let a vehicle pass an older vehicle waiting in the opposite direction.
        if (waitingQueues[1 - direction].count > 0 &&
            nextVehicle->arrival_order >=
            waitingQueues[1 - direction].head->arrival_order) {
            break;
        }

        nextVehicle = DequeueVehicle(&waitingQueues[direction]);

        bridge_direction = direction;
        bridge_count++;

        // Print FCFS vehicles in the order they are admitted.
printf("Time %.1f: Vehicle %d (%c) crossing\n",
       ElapsedTime(), nextVehicle->id,
       DirectionName(nextVehicle->dir));
fflush(stdout);

Sem_Post(&nextVehicle->permission);
    }
}


void CrossBridge(vehicle_info *ti)
{
    // Greedy prints when the vehicle starts crossing.
    // FCFS already prints when permission is granted.
    if (crossing_policy == GREEDY) {
        double startTime = ElapsedTime();

        printf("Time %.1f: Vehicle %d (%c) crossing\n",
               startTime, ti->id, DirectionName(ti->dir));
        fflush(stdout);
    }

    Spin(CROSSING_TIME);
}

void ExitBridge(vehicle_info *ti)
{
    Sem_Wait(&bridgeMutex);

    // One vehicle has finished crossing.
    bridge_count--;

    // If the bridge is empty, reset its direction.
    if (bridge_count == 0) {
        bridge_direction = -1;
    }

    // Greedy: allow waiting vehicles to enter when space is available.
    if (crossing_policy == GREEDY) {
        WakeUpGreedy();
    } else {
        WakeUpFCFS();
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

  // Initialize shared mutex and each vehicle's permission semaphore.
Sem_Init(&bridgeMutex, 1);

for (i = 0; i < numVehicles; i++) {
    Sem_Init(&vehicles[i].permission, 0);
    vehicles[i].arrival_order = i;
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

    for (i = 0; i < numVehicles; i++) {
    Sem_Destroy(&vehicles[i].permission); }

    Sem_Destroy(&bridgeMutex);
    return 0;
}
