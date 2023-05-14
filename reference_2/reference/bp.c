/* 046267 Computer Architecture - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//BTB line
typedef struct
{
	bool valid;
	uint32_t tag;
	uint32_t target;
} BTBlines;
//type of share
typedef enum
{
	NOT_USING_SHARE,
	USING_SHARE_LSB,
	USING_SHARE_MID
} Share;
//state machine
typedef enum 
{
FSM_SNT,
FSM_WNT,
FSM_WT,
FSM_ST
}FSM;
//main struct 
typedef struct
{	
	bool isGlobalHist;
	bool isGlobalTable;
	unsigned btbSize;
	unsigned historySize;
	unsigned DefaultFsmState;
	unsigned tagSize;
	FSM *globalFSM;
	FSM **privateFSM;
	Share sharedType;
	SIM_stats cur_sim_stat;
	uint8_t GHRHistory;
	uint8_t *BHRHistory;
	BTBlines *BTBentries;
} btbvalues;



static btbvalues *btb;


//function that updating the state machine
FSM updateFSM(FSM cur_bimodal, bool taken)
{
	if (taken)
	{
		if(cur_bimodal<3)
		{
			cur_bimodal++;
		}
	}
	else 
	{
		if(cur_bimodal>0)
		{
			cur_bimodal--;
		}
	}

	return cur_bimodal;
}
//creates bit mask
uint32_t bitmask(uint8_t lowe, uint8_t high)
{
	uint32_t allones = -1; //all is one
	uint32_t lowe_mask = (pow(2, lowe) - 1); //from one to lowe is one
	uint32_t high_mask = ((uint32_t)pow(2, high + 1) - 1)^allones; // from high to 32 is one
	uint32_t mask = allones ^ lowe_mask ^ high_mask;
	return mask; // from low to high is one
}
//function that calculate the history 
uint8_t calchistory(bool isGlobalHist,uint32_t entryNum ,Share sharedType,unsigned historySize,uint32_t pc)
{
	uint8_t cur_history;

	if (isGlobalHist)
	{
		cur_history = btb->GHRHistory;
	}
	else
	{
		cur_history = btb->BHRHistory[entryNum];
	}
	uint32_t pc_xor = 0;

	if (sharedType == USING_SHARE_LSB)
	{
		pc_xor = (pc >> 2) & bitmask(0, historySize - 1);
	}
	else if (sharedType == USING_SHARE_MID)
	{
		pc_xor = (pc >> 16) & bitmask(0, historySize - 1);
	}

	return cur_history = pc_xor ^ cur_history;
}
//if input is invalid
void update_invalid(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, uint32_t pc_tag, uint32_t table_index)
{
	btb->BTBentries[table_index].valid = true;
	btb->BTBentries[table_index].target = targetPc;
	btb->BTBentries[table_index].tag = pc_tag;
	uint8_t cur_hist = btb->GHRHistory;

	if (taken)
	{
		btb->cur_sim_stat.flush_num++;
	}
	
	if (btb->isGlobalHist)
	{
		btb->GHRHistory = ((btb->GHRHistory << 1) + taken) & bitmask(0, btb->historySize - 1);
		
	}
	else
	{
		cur_hist=0;
		btb->BHRHistory[table_index] = taken;
	}
	uint32_t pc_to_xor=0;
	if (btb->sharedType == USING_SHARE_LSB)
	{
		pc_to_xor = (pc >> 2) & bitmask(0, btb->historySize - 1);
	}
	else if (btb->sharedType == USING_SHARE_MID)
	{
		pc_to_xor = (pc >> 16) & bitmask(0, btb->historySize - 1);
	}

	cur_hist = pc_to_xor ^ cur_hist;


	if (btb->isGlobalTable)
	{
		btb->globalFSM[cur_hist] = updateFSM(btb->globalFSM[cur_hist], taken);
	}
	else
	{
		for (int i = 0; i < pow(2, btb->historySize); i++)
		{
			
			btb->privateFSM[table_index][i] = btb->DefaultFsmState;
		}
		btb->privateFSM[table_index][cur_hist] =updateFSM(btb->DefaultFsmState,taken);
	}
}
//calculate the theoretic size of the BTB
unsigned calctheorteticsize(unsigned btbSize, unsigned historySize, unsigned tagSize){

	int target_size=30;
	int valid_bit=1;
	unsigned tSize = 0;

	tSize = btbSize*(target_size+tagSize+valid_bit);

	if (btb->isGlobalTable)
	{
		tSize += pow(2, historySize) * 2;
		
			if (btb->isGlobalHist)
			{
				tSize += historySize;
			}
			else
			{
			tSize += historySize * btbSize;
			}
	}
	else
	{
		tSize += pow(2, historySize) * 2 * btbSize;

		if (btb->isGlobalHist)
		{
			tSize += historySize;
		}
		else
		{
			tSize += historySize * btbSize;
		}
	}

	
	return tSize;
}

void btbupdating(btbvalues *btb,unsigned btbSize, unsigned historySize,unsigned tagSize,
						unsigned fsmState,bool isGlobalHist,bool isGlobalTable, int Shared)
	{
	btb->btbSize = btbSize;
	btb->historySize = historySize;
	btb->tagSize = tagSize;
	btb->DefaultFsmState = fsmState;
	btb->isGlobalHist = isGlobalHist;
	btb->isGlobalTable = isGlobalTable;
	btb->sharedType = Shared;
	btb->cur_sim_stat.br_num = 0;
	btb->cur_sim_stat.flush_num = 0;
	}

bool Check_Vaild_And_Tag(bool valid, uint32_t tag_pc,uint32_t tag_btb )
{
	if((valid == false) || (tag_btb != tag_pc))
	{
		return true;
	}
	return false;
}
// calculeting the index
uint32_t GetIndex(uint32_t pc, uint32_t bitmask){

 uint32_t Num= (pc >> 2) & bitmask;
 return Num;

}

int checkconditions(bool taken,uint8_t old_history,uint32_t tag_pc,uint32_t table_index,uint32_t targetPc)
{
	if ((btb->BTBentries[table_index].valid) &&
			(tag_pc == btb->BTBentries[table_index].tag))
	{

		if ((btb->globalFSM[old_history] > 1)) return 1;
		if ((btb->BTBentries[table_index].target != targetPc) || (btb->globalFSM[old_history] < 2))  return 2;
	}
		
	return -1;
}

void update_Global_Table(bool taken,uint8_t old_history,uint32_t tag_pc,uint32_t table_index,uint32_t targetPc)
{
	if (!taken)
	{
		if (checkconditions(taken,old_history, tag_pc,table_index, targetPc) ==1)
		
			btb->cur_sim_stat.flush_num++;
		
	}

	else
	{
		if (checkconditions(taken,old_history, tag_pc,table_index, targetPc) ==2)
			btb->cur_sim_stat.flush_num++;
		
	}
	btb->BTBentries[table_index].target=targetPc;
	btb->globalFSM[old_history] = updateFSM(btb->globalFSM[old_history], taken);

}
void update_Local_Table(bool taken,uint8_t old_history,uint32_t tag_pc,uint32_t table_index,uint32_t targetPc)
{
	if (taken)
	{
		if ((!btb->BTBentries[table_index].valid) ||
			(btb->BTBentries[table_index].target != targetPc) ||
			(btb->privateFSM[table_index][old_history] < 2) ||
			(tag_pc != btb->BTBentries[table_index].tag))
		{
			{
				btb->cur_sim_stat.flush_num++;
			}
		}
	}
	else
	{
		if ((btb->BTBentries[table_index].valid) &&
			(tag_pc == btb->BTBentries[table_index].tag) &&
			(btb->privateFSM[table_index][old_history] > 1))
		{
			btb->cur_sim_stat.flush_num++;
		}
	}

	btb->BTBentries[table_index].target=targetPc;	
	btb->privateFSM[table_index][old_history] = updateFSM(btb->privateFSM[table_index][old_history], taken);

}

/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return <0
 */

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
	{
	//to initalize the table and updadint
	btb = (btbvalues *)malloc(sizeof(btbvalues));
	btbupdating(btb,btbSize, historySize,tagSize,fsmState,isGlobalHist,isGlobalTable,Shared);
	//to initalize the history 
	
	if (!isGlobalHist)
	{
		btb->BHRHistory = (uint8_t *)malloc(btbSize * sizeof(uint8_t));
		for (int i = 0; i < btbSize; i++)
		{
			btb->BHRHistory[i] = 0;
		}
	}
	else
	{
		btb->GHRHistory = 0;
	}
	//to initalize the FSM
	int FsmArraySize = (int)(pow(2, historySize));
	if (!isGlobalTable)
	{
		btb->privateFSM = (FSM **)malloc(btbSize * sizeof(FSM *));
		int i=0;
		while (i < btbSize)
		{
			btb->privateFSM[i] = (FSM *)malloc(FsmArraySize * sizeof(FSM));
			for (int j = 0; j < FsmArraySize; j++)
			{
				btb->privateFSM[i][j] =btb->DefaultFsmState;
			}
		i++;	
		}
	}
	else
	{
		btb->globalFSM = (FSM *)malloc(FsmArraySize * sizeof(FSM));
		int i = 0;
		while (i < FsmArraySize){
			btb->globalFSM[i] =btb->DefaultFsmState;
			i++;
		}
	
	}
	//to initalize the Lines insid the FSM	
	btb->BTBentries = (BTBlines *)malloc(btbSize * sizeof(BTBlines));
		int i = btbSize;
		while (i<btbSize)
		btb->BTBentries[i].valid = false;
		i--;
		

	//calculat the size
	btb->cur_sim_stat.size = calctheorteticsize(btbSize, historySize, tagSize);

	return 0; 
	}
/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */

bool BP_predict(uint32_t pc, uint32_t *dst){

  //calculat a mask and the index to serch on the table
	uint32_t mask = bitmask(0, (log(btb->btbSize) / log(2)) - 1);

	if (btb->btbSize == 1)
	{
		mask = 0;
	}
	
	uint32_t table_index = GetIndex(pc, mask);
  //calculate the tag
	uint32_t shift_pc = pc >> 2;
	uint32_t tag_pc = shift_pc >> ((int)(log(btb->btbSize) / log(2)));
	tag_pc = tag_pc & bitmask(0, btb->tagSize - 1);

  //chack if the input is valid
	if (btb->BTBentries[table_index].valid == false || btb->BTBentries[table_index].tag != tag_pc)
	{
		*dst = pc + 4;
		return false;
	}

	uint8_t cur_history;
	
	//chack what is the relevent history acoording to the input
	cur_history=calchistory(btb->isGlobalHist,table_index,btb->sharedType,btb->historySize,pc);

	//updating the FSM according to the relevent history

	FSM relevant_FSM;

	if (btb->isGlobalTable)
	{
		relevant_FSM = btb->globalFSM[cur_history];
	}
	else 
	{
		relevant_FSM = btb->privateFSM[table_index][cur_history];
	}

	//updating the *dest
	if (relevant_FSM < 2)
	{
		*dst = pc + 4;
		return false;
	}
	*dst = btb->BTBentries[table_index].target;

	return true;
}
/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the branch instruction target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 * param[in] pred_dst - the predicted target address
 */

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	uint8_t old_history;
	btb->cur_sim_stat.br_num++;
	uint32_t mask = bitmask(0, (log(btb->btbSize) / log(2)) - 1);
	if (btb->btbSize == 1)
	{
		mask = 0;
	}
	uint32_t table_index = GetIndex(pc, mask);
	uint32_t shift_pc = pc >> 2;
	uint32_t tag_pc = shift_pc >> ((int)(log(btb->btbSize) / log(2)));
	tag_pc = tag_pc & bitmask(0, btb->tagSize - 1);
	old_history=calchistory(btb->isGlobalHist,table_index,btb->sharedType,btb->historySize,pc);

	if (btb->BTBentries[table_index].valid == false || btb->BTBentries[table_index].tag != tag_pc)
	{
		update_invalid(pc, targetPc, taken, pred_dst, tag_pc, table_index);
		return;
	}

	if (btb->isGlobalHist)
	{
		btb->GHRHistory = ((btb->GHRHistory << 1) + taken) & bitmask(0, btb->historySize - 1);
	}
	else
	{
		btb->BHRHistory[table_index] = ((btb->BHRHistory[table_index] << 1) + taken) & bitmask(0, btb->historySize - 1);
	}


	if (btb->isGlobalTable)
	{
		update_Global_Table(taken,old_history,tag_pc,table_index,targetPc);
	}

	else
	{
		update_Local_Table(taken,old_history,tag_pc,table_index,targetPc);
	}

	return;
}

/*
 * BP_GetStats: Return the simulator stats using a pointer
 * curStats: The returned current simulator state (only after BP_update)
 */
void BP_GetStats(SIM_stats *curStats)
{
	*curStats = btb->cur_sim_stat;

	//free all the malloc

	free(btb->BTBentries); 

	if (btb->isGlobalTable)
	{
		free(btb->globalFSM); 
	}
	else
	{

		for (int i = 0; i < btb->btbSize; i++)
		{
			free(btb->privateFSM[i]);
		}
		free(btb->privateFSM);

	}

	if (!(btb->isGlobalHist))
	{
		free(btb->BHRHistory);
	}

	free(btb);

	return;

}



