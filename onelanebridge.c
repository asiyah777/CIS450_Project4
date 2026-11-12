// In Gallap Park in Ann Arbor, there is a one-lane bridge. Vehicles arriving from the north
// and south must share this single lane. Vehicles heading in the same direction may cross
// simultaneously, but vehicles heading in opposite directions cannot. Additionally, if more
// than three vehicles are on the bridge at once, it will collapse.

typedef struct _vehicle_info {
    int id; // vehicle ID
    int dir; // heading direction (N or S)
    double inter_arrival_t; // inter-arrival time before next vehicle
} vehicle_info; 



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

    return 0;
}