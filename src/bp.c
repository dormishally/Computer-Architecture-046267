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

/********************************  FSM  **********************************/
typedef enum {SNT, WNT, WT, ST} FSM_STATE;

uint32_t fsmTableSize;

FSM_STATE FSM_Update(FSM_STATE curr_state, bool isTaken){
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
    return curr_state;
} 

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
    bool isGlobalHist;
    bool isGlobalTable;
    SHARE_TYPE sharedType;

    FSM_STATE *FSM_global;
    FSM_STATE **FSM_local;
    BTB_entry *btb_entry;
    SIM_stats sim_stats;
    uint8_t GHR;
    uint8_t *BHR;
}BTB_t;

BTB_t *btb = NULL;

/********************************  functions  ***********************************/
uint32_t Mask_Calc(uint32_t x){
    uint32_t ffff = -1;
    uint32_t low = 0;
    uint32_t high = ffff ^ ((uint32_t)pow(2, x) -1);
    uint32_t tmp = ffff ^ low ^ high;
    return tmp;
}

unsigned Sim_Stats_Size(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, bool isGlobalTable){
    unsigned hist_n = (isGlobalHist) ? 1 : btbSize;
    unsigned table_n = (isGlobalTable) ? 1 : btbSize;
    return (btbSize * (tagSize + 30 + 1)) + (hist_n * historySize) + (table_n * 2 * pow(2, historySize));  
}

int BTB_Init(BTB_t *btb, unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
                    bool isGlobalHist, bool isGlobalTable, int Shared){
    btb->BTB_size = btbSize;
    btb->hist_size = historySize;
    btb->tag_size = tagSize;
    btb->init_state = fsmState;
    btb->isGlobalHist = isGlobalHist;
    btb->isGlobalTable = isGlobalTable;
    btb->sharedType = Shared;
    btb->sim_stats.flush_num = 0;
    btb->sim_stats.br_num = 0;
    btb->sim_stats.size = Sim_Stats_Size(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable);
    btb->btb_entry = (BTB_entry*)malloc(sizeof(BTB_entry));
    if(btb->btb_entry == NULL){
        return -1;
    }
    return 0;
}

int Table_Init(double table_size){
    if(btb->isGlobalTable){
        btb->FSM_global = (FSM_STATE*)malloc(table_size*sizeof(FSM_STATE));
        if(btb->FSM_global == NULL){
            return -1;
        }
        for(int i=0; i<table_size; i++){
            btb->FSM_global[i] = btb->init_state;
        }
    }
    else{
        btb->FSM_local = (FSM_STATE**)malloc(table_size*sizeof(FSM_STATE*));
        if(btb->FSM_local == NULL){
            return -1;
        }
        for(int i=0; i<table_size; i++){
            btb->FSM_local[i] = (FSM_STATE*)malloc(table_size*sizeof(FSM_STATE));
            if(btb->FSM_local[i] == NULL){
            return -1;
        }
        }
        for(int i=0; i<table_size; i++){
            for(int j=0; j<table_size; j++){
                btb->FSM_local[i][j] = btb->init_state;
            }
        }
    }
    return 0;
}

int Hist_Init(int size){
    if(btb->isGlobalHist){
        btb->GHR = 0;
    }
    else{
        btb->BHR = (uint8_t*)malloc((size)*sizeof(uint8_t));
        if(btb->BHR == NULL){
            return -1;
        }
        for(int i=0; i<size; i++){
            btb->BHR[i] = 0;
        }
    }
    return 0;
}


void Table_Update(bool isTaken, uint32_t indx, uint8_t hist, uint32_t tag, uint32_t targetPc){
    
    bool btbValid = btb->btb_entry[indx].valid;
    uint32_t btbTag = btb->btb_entry[indx].tag;
    uint32_t btbTarget = btb->btb_entry[indx].target;

    if(btb->isGlobalTable){//global table
        if(isTaken){
           if(btbValid && (btbTag == tag)){
                if(btbTarget != targetPc || btb->FSM_global[hist] < 2){
                    btb->sim_stats.flush_num ++;
                }
            }
               
        }
        else{
            if(btbValid && (btbTag == tag)){
                if(btb->FSM_global[hist] > 1){
                    btb->sim_stats.flush_num ++;
                }
            }
        }
        btb->FSM_global[hist] = FSM_Update(btb->FSM_global[hist], isTaken);
    }
    else{////////////////////local table
        if(isTaken){
            if(!(btbValid) || (btbTag != tag) || (btb->FSM_local[indx][hist] < 2) || (btb->btb_entry[indx].target != targetPc)){
                btb->sim_stats.flush_num ++;
            }               
        }
        else{
            if(btbValid && (btbTag == tag) && (btb->FSM_local[indx][hist] > 1)){
                btb->sim_stats.flush_num ++;
            }
            
        }
        btb->FSM_local[indx][hist] = FSM_Update(btb->FSM_local[indx][hist], isTaken);
    }
    btb->btb_entry[indx].target = targetPc;
}

void Hist_Update(uint32_t indx ,bool isTaken){
    uint32_t mask = Mask_Calc(btb->hist_size);
    if(btb->isGlobalHist){
        btb->GHR = ((((btb->GHR) << 1) + isTaken) & mask);
    }
    else{
        btb->BHR[indx] = ((((btb->BHR[indx]) << 1) + isTaken) & mask);
    }
}


uint8_t SharedType_Hist(uint32_t pc, uint32_t indx){
    uint32_t xor_calc = 0;
    uint8_t hist = (btb->isGlobalHist)? (btb->GHR) : (btb->BHR[indx]);
    if(btb->sharedType == USING_SHARE_LSB){
        xor_calc = (pc >> 2) & Mask_Calc(btb->hist_size);
    }
    if(btb->sharedType == USING_SHARE_MID){
        xor_calc = (pc >> 16) & Mask_Calc(btb->hist_size);
    }
    return hist ^ xor_calc;
}


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	btb = (BTB_t*)malloc(sizeof(BTB_t));
    if(btb == NULL){
        return -1;
    } 
    int init_success = 0;
    init_success += BTB_Init(btb, btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
    init_success += Table_Init(pow(2, historySize));
    init_success += Hist_Init((int)btbSize);

    for(int i=0; i<btbSize; i++){
        btb->btb_entry[i].valid = false;
    }
 
    return init_success;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
    uint32_t mask = ((btb->BTB_size) == 1)? 0: Mask_Calc(log2(btb->BTB_size));
    //uint32_t indx = ((pc >> 2) & Mask_Calc(log2(btb->BTB_size)));
    uint32_t indx = ((pc >> 2) & mask);
    //uint32_t tag = ((pc >> 2) >> (uint32_t)log2(btb->BTB_size)) & Mask_Calc(btb->BTB_size);
    uint32_t tag = ((pc >> 2) >> (uint32_t)log2(btb->BTB_size)) & mask;
    uint8_t curr_hist = SharedType_Hist(pc, indx);
    
    bool btbValid = btb->btb_entry[indx].valid;
    uint32_t btbTag = btb->btb_entry[indx].tag;
    if(!btbValid || (btbTag != tag)){
        *dst = pc + 4;
        return false;
    }
    //printf("indx = %d, tag = %d, cur_hist = %d\n", indx, tag, curr_hist);
    FSM_STATE cur_state = (btb->isGlobalTable)? (btb->FSM_global[curr_hist]) : (btb->FSM_local[indx][curr_hist]);

    if(cur_state == SNT || cur_state == WNT){
        *dst = pc + 4;
        return false;
    }
    *dst = btb->btb_entry[indx].target;
	return true;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    uint32_t mask = ((btb->BTB_size) == 1)? 0: Mask_Calc(log2(btb->BTB_size));
    btb->sim_stats.br_num++;
    uint32_t indx = ((pc >> 2) & mask);
    uint32_t tag = ((pc >> 2) >> (uint32_t)log2(btb->BTB_size)) & mask;
    uint8_t curr_hist = SharedType_Hist(pc, indx);
    bool btbValid = btb->btb_entry[indx].valid;
    uint32_t btbTag = btb->btb_entry[indx].tag;

    if(!btbValid || (btbTag != tag)){
        btb->btb_entry[indx].valid = true;
        btb->btb_entry[indx].target = targetPc;
        btb->btb_entry[indx].tag = tag;
        uint8_t cur_hist = (btb->isGlobalHist)? (btb->GHR) : 0;

        if(taken){
            btb->sim_stats.flush_num ++;
        }

        if(btb->isGlobalHist){
            btb->GHR = ((btb->GHR << 1) + taken) & Mask_Calc(btb->hist_size);
        }
        else{
            btb->BHR[indx] = taken;
        }

        uint32_t pc_xor = 0;
        if(btb->sharedType == USING_SHARE_LSB){
            pc_xor = (pc >> 2) & Mask_Calc(btb->hist_size);
        }
        else if(btb->sharedType == USING_SHARE_MID){
            pc_xor = (pc >> 16) & Mask_Calc(btb->hist_size);
        }

        cur_hist = pc_xor ^ cur_hist;

        if (btb->isGlobalTable){
		    btb->FSM_global[cur_hist] = FSM_Update(btb->FSM_global[cur_hist], taken);
	    }
	    else{
		    for (int i = 0; i < pow(2, btb->hist_size); i++)
		    {
			
			    btb->FSM_local[indx][i] = btb->init_state;
		    }
		    btb->FSM_local[indx][cur_hist] =FSM_Update(btb->init_state,taken);
	    }
        return;
    }
    Hist_Update(indx, taken);
    Table_Update(taken, indx, curr_hist, tag, targetPc);
}
/*
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	btb->sim_stats.br_num++;
    uint32_t indx = ((pc >> 2) & Mask_Calc(log2(btb->BTB_size)));
    uint32_t tag = ((pc >> 2) >> (uint32_t)log2(btb->BTB_size)) & Mask_Calc(btb->BTB_size);
    uint8_t curr_hist = SharedType_Hist(pc, indx);

    bool btbValid = btb->btb_entry[indx].valid;
    uint32_t btbTag = btb->btb_entry[indx].tag;
    
    Table_Update(taken, indx, curr_hist, tag, targetPc);

    if(btbValid && (btbTag == tag)){
        Table_Update(taken, indx, curr_hist, tag, targetPc);
        Hist_Update(indx, taken);
    }
    else{
        btb->btb_entry[indx].valid = true;
        btb->btb_entry[indx].target = targetPc;
        btb->btb_entry[indx].tag = tag;
        //uint8_t cur_hist = btb->GHR;

        if(taken){
            btb->sim_stats.flush_num++;
        }


        if (btb->isGlobalHist){
            btb->GHR = ((((btb->GHR) << 1) + taken) & Mask_Calc(btb->hist_size));
        }
        else{
            ///cur_hist = 0;
            btb->BHR[indx] = taken;
        } 

        uint32_t xor_calc = 0;
        uint8_t hist = (btb->isGlobalHist)? (btb->GHR) : (0);
        if(btb->sharedType == USING_SHARE_LSB){
            xor_calc = (pc >> 2) & Mask_Calc(btb->hist_size);
        }
        if(btb->sharedType == USING_SHARE_MID){
            xor_calc = (pc >> 16) & Mask_Calc(btb->hist_size);
        }
        hist = hist ^ xor_calc;
        if (btb->isGlobalTable)
	    {
		    btb->FSM_global[hist] = FSM_Update(btb->FSM_global[hist], taken);
	    }
        else{
            int size = pow(2, btb->hist_size);
            for(int i=0; i<size; i++){
                btb->FSM_local[indx][i] = btb->init_state;
            }
            btb->FSM_local[indx][hist] = FSM_Update(btb->FSM_local[indx][hist], taken);
        }     
    }
	return;
}
*/

void BP_GetStats(SIM_stats *curStats){
    *curStats = btb->sim_stats;
    if(!(btb->isGlobalTable)){
        for(int i=0; i<btb->BTB_size; i++){
            free(btb->FSM_local[i]);
        }
        free(btb->FSM_local);
    }
    else{
        free(btb->FSM_global);
    }
    free(btb->btb_entry);

    if(!(btb->isGlobalHist)){
        free(btb->BHR);
    }
    free(btb);
	return;
}

