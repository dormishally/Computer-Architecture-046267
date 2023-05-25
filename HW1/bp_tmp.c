/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_BTB_SIZE 32
#define MAX_STATE_MACHINES = 256 // 2^8 (8 = max size of history)
#define COLUMNS_BTB = 4
#define ADDRESS_SIZE = 30
#define LOCAL 0
#define GLOBAL 1

typedef enum {NOT_USING_SHARE, USING_SHARE_LSB, USING_SHARE_MID} SHARE_TYPE;
//typedef enum {GLOBAL, LOCAL} ;

/********************************  FSM  **********************************/
typedef enum {SNT, WNT, WT, ST} FSM_STATE;

uint32_t fsmTableSize;

FSM_STATE update_state(FSM_STATE curr_state, bool isTaken)(
    if(curr_state == SNT){
        return (isTaken)? WNT : SNT;
    }
    if(curr_state == WNT){
        return (isTaken)? WT : SNT;
    }    
    if(curr_state == WT){
        return (isTaken)? ST : WNT;
    }    
    if(curr_state == ST){
        return (isTaken)? ST : WT;
    }
    return -1;
) 

/********************************  BTB  ***********************************/
typedef struct{
	bool valid;
	uint32_t tag;
	uint32_t target;
} BTB_entry;

typedef struct{
    unsigned BTB_size;
    unsigned hist_size;
    unsigned tag_size;
    unsigned init_state;
    bool global_hist;
    bool global_table;
    SHARE_TYPE sharedType;

    FSM_STATE *FSM_global;
    FSM_STATE **FSM_local;
    BTB_entry *btb_entry;
    SIM_stats sim_stats;
    uint8_t GHR;
    uint8_t *BHR;
}BTB_t

/////// FUNCTIONS //////


uint32_t bitmask(uint8_t low_bit, uint8_t high_bit){
    uint32_t one_base = -1;
    uint32_t lwo_mask = pow(2, lower) - 1;
    uint32_t high_mask = one_base ^ ((uint32_t)pow(2, high_bit+1)-1);
    return one_base ^ lwo_mask ^ high_mask;
}

unsigned sim_stats_size(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, bool isGlobalTable){
    unsigned hist_n = (isGlobalHist) ? 1 : btbSize;
    unsigned table_n = (isGlobalTable) ? 1 : btbSize;
    return (btbSize * (tagSize + 30 + 1)) + (hist_n * historySize) + (table_n * 2 * pow(2, historySize));  
}

void BTB_init(BTB_t *btb, unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
                    bool isGlobalHist, bool isGlobalTable, int Shared){
    btb->BTB_size = btbSize;
    btb->hist_size = historySize;
    btb->tag_size = tagSize;
    btb->init_state = fsmState;
    btb->global_hist = isGlobalHist;
    btb->global_table = isGlobalTable;
    btb->sharedType = Shared;
    btb->sim_stats->flush_num = 0;
    btb->sim_stats->br_num = 0;
    btb->sim_stats->size = sim_stats_size(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable);
    btb->btb_entry = (BTB_entry*)malloc(sizeof(BTB_entry));
}

void tables_init(BTB_t *btb, unsigned btbSize, unsigned historySize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable){
    if(isGlobalHist){
        btb->GHR = 0;
    }
    else{ //history is local
        btb->BHR = (uint8_t*)malloc(btbSize * sizeof(uint8_t));
        for(int i=0; i<btbSize; i++){
            btb->BHR[i] = 0;
        }
    }

    int fsmSize = pow(2, historySize);
    if(isGlobalTable){
        btb->FSM_global = (FSM_STATE*)malloc(fsmSize * sizeof(FSM_STATE));
        for(int i=0; i<fsmSize; i++){
            btb->FSM_global[i] = fsmState;
        }
    }

}


//enum BIMODAL{USING_SHARE_LSB = 1, USING_SHARE_MID = 2, NOT_USING_SHARE=0 };
//enum SHARE_TYPE{GLOBAL, LOCAL};

//enum FSM_STATE{SNT = 0, WNT = 1, WT = 2, ST = 3};


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	btb = (BTB_t*)malloc(sizeof(BTB_t));
    BTB_init(btb, btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
    


    
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


