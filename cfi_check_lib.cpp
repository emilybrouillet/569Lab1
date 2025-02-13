#include <iostream>
#include <unordered_map>
#include <unordered_set>

std::unordered_map<int, std::unordered_set<int>> ValidTransitions;

int currentBBID = -1;

extern "C" void initCFG() {

    // This is hardcoded for simplicity
    ValidTransitions[0].insert({1,2});
    ValidTransitions[1].insert(2);
    ValidTransitions[2].insert({3,4,5});
    ValidTransitions[3].insert(-1);  //-1 is no further jumps
    ValidTransitions[4].insert(-1); 
    ValidTransitions[5].insert(-1);

}

// Runtime CFI check function
extern "C" void __cfi_check_bb(int targetBBID) {
    
    if (ValidTransitions[currentBBID].count(targetBBID)) { 
        extern "C" void __cfi_check_bb(int targetBBID) {
    if (ValidTransitions[currentBBID].count(targetBBID)) {
        currentBBID = targetBBID;
    } else {
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
