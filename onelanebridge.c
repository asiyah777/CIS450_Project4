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

void ArriveBridge(vehicle_info *ti);

void CrossBridge(vehicle_info *ti);

void ExitBridge(vehicle_info *ti);

int main() {

    return 0;
}