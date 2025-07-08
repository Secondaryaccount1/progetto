
#include <assert.h>
#include <stdio.h>
#include "utils.h"

int main(void) {
    // Test della distanza di Manhattan
    assert(manhattan_distance(0,0, 3,4) == 7);
    assert(manhattan_distance(-1,2, 2,-3) == 8);

    // Tempo di viaggio = distanza / velocità
    rescuer_type_t r1;
    r1.speed = 5;  // velocità = 5 unità/sec
    double tt = travel_time_secs(&r1, 0,0, 5,0);
    assert(tt == 1.0);  // 5/5 = 1s

    // Test delle scadenze
    assert(priority_deadline_secs(2) == 10);
    assert(priority_deadline_secs(1) == 30);
    assert(priority_deadline_secs(0) == 2147483647);

    printf("[test_utils] all tests passed\n");
    return 0;
}

