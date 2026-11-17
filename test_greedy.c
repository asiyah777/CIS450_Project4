
#include <stdio.h>

// These variables represent the current bridge state.
static int bridge_direction = -1;
static int bridge_count = 0;
static int south_waiting_count = 0;

// Same Greedy rules used in onelanebridge.c.
// This is a separate test so we do not change the team's code.
int CanEnterGreedy(int direction) {
    if (bridge_count == 0) {
        return 1;
    }

    if (bridge_count < 3 && bridge_direction == direction) {
        return 1;
    }

    return 0;
}

int main(void) {
    // Test 1: Empty bridge
    bridge_count = 0;
    bridge_direction = -1;
    printf("Test 1 (empty bridge): %s\n",
           CanEnterGreedy(0) ? "PASS" : "FAIL");

    // Test 2: Two vehicles going the same direction
    bridge_count = 2;
    bridge_direction = 0;
    printf("Test 2 (same direction): %s\n",
           CanEnterGreedy(0) ? "PASS" : "FAIL");

    // Test 3: Bridge already has three vehicles
    bridge_count = 3;
    bridge_direction = 0;
    printf("Test 3 (bridge full): %s\n",
           !CanEnterGreedy(0) ? "PASS" : "FAIL");

    // Test 4: Vehicle going the opposite direction
    bridge_count = 2;
    bridge_direction = 0;
    printf("Test 4 (opposite direction): %s\n",
           !CanEnterGreedy(1) ? "PASS" : "FAIL");

    
// Test 5: Greedy allows entry even when opposite vehicles are waiting.
// Two northbound vehicles are crossing, and two southbound vehicles are waiting.
bridge_count = 2;
bridge_direction = 0;
south_waiting_count = 2;

printf("Test 5 (opposite vehicles waiting): %s\n",
       CanEnterGreedy(0) ? "PASS" : "FAIL");


    return 0;
}
