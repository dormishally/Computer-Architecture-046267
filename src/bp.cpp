/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <memory>
#include <vector>
#include <list>
#include <algorithm>
#include <math.h>
using namespace std;

enum BIMODAL{USING_SHARE_LSB = 1, USING_SHARE_MID = 2, NOT_USING_SHARE=0 };
enum SHARE_TYPE{GLOBAL, LOCAL};
const int MAX_BTB_SIZE = 32;
const int  MAX_STATE_MACHINES = 256; // 2^8 (8 = max size of history)
const int COLUMNS_BTB = 4; // for [0] = valid bit ,[1] = tag and [2] = history, [3] = jmp_ip
const int ADDRESS_SIZE = 30;
enum FSM_STATE{SNT = 0, WNT = 1, WT = 2, ST = 3};

/*---------------------------------Class FSM-----------------------------------*/
class FSM {
private:
    FSM_STATE fsmState;
public:
    FSM(uint32_t initial_state) :
            fsmState(FSM_STATE(initial_state)) {}

    bool predict() {
        return (fsmState == WT || fsmState == ST);
    }
    bool getState() {
        return (fsmState >= 2);
    }
    int get_state_int() {
        if (fsmState == ST) {
            return 3;
        } else if (fsmState == SNT) {
            return 0;
        } else if (fsmState == WNT) {
            return 1;
        } else {       //if (fsmState == WT)
            return 2;
        }
    }
    FSM_STATE &operator++();
    FSM_STATE &operator--();
};
/*------------------------------------BTB------------------------------------------------*/


/*------------------------------------BP------------------------------------------------*/
class BP {
public:
    uint32_t BTB_size;
    uint32_t history_reg_size;
    uint32_t tag_size;
    BIMODAL using_share_type;
    FSM start_state;
    SHARE_TYPE history_type;
    SHARE_TYPE state_machine_type;
    std::vector<std::vector<uint32_t>> history_cache;
    uint32_t global_history = 0;
    std::vector<std::vector<FSM>> local_state_machine_array;
    std::vector<FSM> global_state_machine_array;
    int branch_counter = 0;
    int wrong_prediction_counter = 0;

    // functions
    BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
       bool isGlobalHist, bool isGlobalTable, int Shared) :
            BTB_size(btbSize),
            history_reg_size(historySize),
            tag_size(tagSize),
            using_share_type(BIMODAL(Shared)),
            start_state(FSM_STATE(fsmState))
    {
        history_type = (isGlobalHist ? GLOBAL : LOCAL);
        state_machine_type = (isGlobalTable ? GLOBAL : LOCAL);
    } //c'tor
    ~BP() = default; // d'tor
    BP(BP& other) = default; // copy c'tor
};

BP* bp_pointer;
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared){
    return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
    return false;
}
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    return;
}

void BP_GetStats(SIM_stats *curStats){
    return;
}
