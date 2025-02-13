#include <iostream>
#include <unordered_map>
#include <unordered_set>

// Map from basic block IDs to allowed target IDs
std::unordered_map<int, std::unordered_set<int>> ValidTransitions;

// Global variable to track the current basic block ID
int currentBBID = -1;

// Function to add valid transitions
extern "C" void initCFG() {

    // This is hardcoded for simplicity
    ValidTransitions[0].insert({1,2});
    ValidTransitions[1].insert(2);
    ValidTransitions[2].insert({3,4,5});
    ValidTransitions[3].insert(-1);  //-1 is no further jumps
    ValidTransitions[4].insert(-1); 
    ValidTransitions[5].insert(-1);

    /* Insert more valid targets */
    /* . . . */
}

// Runtime CFI check function
extern "C" void __cfi_check_bb(int targetBBID) {
    /* iterate the data structure `ValidTransitions` */
    /* check if <CurrentBBID, targetBBID> is in `ValidTransitions`  */
    
    if (ValidTransitions[currentBBID].count(targetBBID)) { 
        // Allowed transition, update current basic block ID currentBBID = targetBBID; } 
        // Update the current basic block ID
        extern "C" void __cfi_check_bb(int targetBBID) {
    if (ValidTransitions[currentBBID].count(targetBBID)) {
        // Allowed transition, update current basic block ID
        currentBBID = targetBBID;
    } else {
        // Check if the current block is terminal
        if (ValidTransitions[currentBBID].count(-1)) {
            printf("CFI Violation: No valid transitions allowed from BB-%d!\n", currentBBID);
            exit(1);
        } else {
            printf("CFI Violation: Invalid transition from BB-%d to BB-%d!\n", currentBBID, targetBBID);
            exit(1);
        }
    }
}
}  
