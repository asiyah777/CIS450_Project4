#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// In Gallap Park in Ann Arbor, there is a one-lane bridge. Vehicles arriving from the north
// and south must share this single lane. Vehicles heading in the same direction may cross
// simultaneously, but vehicles heading in opposite directions cannot. Additionally, if more
// than three vehicles are on the bridge at once, it will collapse.

typedef struct _vehicle_info {
    int id; // vehicle ID
    int dir; // heading direction (N or S)
    double inter_arrival_t; // inter-arrival time before next vehicle
} vehicle_info; 

// Semaphore 
typedef struct _waiting_vehicle {
    // TODO: Add fields to represent state waiting vehcicles
} waiting_vehicle;

// Queues for vehicles waiting to cross the bridge
waiting_vehicle north_waiting[13];
waiting_vehicle south_waiting[13];

void enqueue(waiting_vehicle *queue, vehicle_info *vehicle) {
}

void dequeue(waiting_vehicle *queue, vehicle_info *vehicle) {
}

void* VehicleAction(void* arg) {
    vehicle_info *ti = (vehicle_info *) arg;
    ArriveBridge(ti);
    CrossBridge(ti);
    ExitBridge(ti);
}

//Blocks the calling vehicle until it is safe to cross. There are two
// crossing policies:
// • Greedy Crossing: A vehicle may enter the bridge if fewer than 3 vehicles are
// currently on it and all are heading the same direction — even if vehicles in the
// opposite direction are waiting. This may cause starvation.
// • FCFS Crossing: A vehicle may enter the bridge if fewer than 3 vehicles are on it, all
// head the same direction, and no vehicle is waiting in the opposite direction. All
// vehicles are served in first-come, first-served order.
void ArriveBridge(vehicle_info *ti);

// Simulates crossing time. Crossing takes a fixed 5 seconds, simulated
//using the Spin() function from the OSTEP code package.
void CrossBridge(vehicle_info *ti);

// Called when a vehicle finishes crossing. Updates shared state to allow
// waiting vehicles to proceed.
void ExitBridge(vehicle_info *ti);

void Spin(int howlong) {
    double t = GetTime();
    while ((GetTime() - t) < (double)howlong)
	; // do nothing in loop
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <crossing_policy>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "greedy") == 0) {
        printf("Using greedy crossing policy\n");
    } else if (strcmp(argv[1], "fcfs") == 0) {
        printf("Using FCFS crossing policy\n");
    } else {
        fprintf(stderr, "Invalid crossing policy: %s\n", argv[1]);
        exit(1);
    }

    vehicle_info vehicles[13] = {
        {0, 0.0, 'N'},
        {1, 1.0, 'N'},
        {2, 2.0, 'S'},
        {3, 3.0, 'S'},
        {4, 4.0, 'S'},
        {5, 5.0, 'N'},
        {6, 6.0, 'N'},
        {7, 7.0, 'S'},
        {8, 8.0, 'S'},
        {9, 9.0, 'N'},
        {10, 10.0, 'N'},
        {11, 11.0, 'N'},
        {12, 12.0, 'N'}
    };

    return 0;
}