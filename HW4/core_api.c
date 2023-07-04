#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <stdbool.h>

#define AWAKE_COUNTER -2
#define REGS_COUNT 8

typedef struct {
    bool active_thread;
    int state_counter;
    int cur_inst;
    int inst_cont_one_thread;
    tcontext context;
} thread_context;

typedef struct {
    thread_context *threads;
    int cycles_count;
    int inst_count;
    int threads_num;
} MT_cpu_t;

static MT_cpu_t * blocked_MT;
static MT_cpu_t * finegrained_MT;


static bool isAnyActiveThread(MT_cpu_t* cpu,int threadnumber)
{
    int cur_thread = 0;
    while (cur_thread<threadnumber)
    {
        if (cpu->threads[cur_thread].active_thread==true)
        {
            return true;
        }
        cur_thread++;
    }

    return false;
}
static int Mod (int num1, int num2)
{
    return num1 % num2;
}

static bool IsAnyAwakeThread(MT_cpu_t* cpu,int threadnumber)
{
    int cur_thread = 0;
    while (cur_thread<threadnumber)
    {
        if (cpu->threads[cur_thread].state_counter != -1)
        {
            return true;
        }
        cur_thread++;
    }

    return false;
}

static void initCpu(MT_cpu_t* MT_cpu,int threadnumber)
{
    MT_cpu->threads = malloc(sizeof(thread_context) * threadnumber);
	if(MT_cpu->threads == NULL){
		return;
	}
    MT_cpu->cycles_count=0;
    MT_cpu->inst_count=0;

    int cur_thread = 0;
    while (cur_thread<threadnumber)
    {
        //first- all threads are actives
        MT_cpu->threads[cur_thread].active_thread = true;
        MT_cpu->threads[cur_thread].cur_inst = 0;
        MT_cpu->threads[cur_thread].state_counter = AWAKE_COUNTER;
        //for all the registers for the cur thread
        for (int i = 0; i < REGS_COUNT; ++i)
        {
            MT_cpu->threads[cur_thread].context.reg[i] = 0;
        }
        cur_thread++;
    }
	return;
}

static void freeCpu(MT_cpu_t* MT_cpu,int threadnumber)
{
	free(MT_cpu->threads);
	free(MT_cpu);
	return;
}


static int GetCurActiveThread(MT_cpu_t* MT_cpu,int cur_thread,int threadnumber)
{
    if (MT_cpu->threads[cur_thread].active_thread)
    {
        return cur_thread;
    }
    int i = Mod((cur_thread+1), threadnumber);
    while (MT_cpu->threads[i].active_thread == false && i != cur_thread)
    {
        i = Mod((i+1),threadnumber);
    }
    return i;
}

static void threadsWakeup(MT_cpu_t* MT_cpu,int threadnumber)
{
    int cur_thread = 0;
    while (cur_thread<threadnumber)
    {
        if (MT_cpu->threads[cur_thread].active_thread == false && MT_cpu->threads[cur_thread].state_counter > 0) 
            MT_cpu->threads[cur_thread].state_counter--;
		
        if (MT_cpu->threads[cur_thread].state_counter == 0) {
            MT_cpu->threads[cur_thread].active_thread = true;
            MT_cpu->threads[cur_thread].state_counter = AWAKE_COUNTER;
        }
        cur_thread++;
    }
	return;
}

static void hendelStoreAndLoad(MT_cpu_t * cpu,int curthread, Instruction *inst,bool isLoad)
{
	uint32_t mem_location = 0;
	if(isLoad == true)
	{
		mem_location = cpu->threads[curthread].context.reg[(*inst).src1_index];
	}
	else{
		mem_location = cpu->threads[curthread].context.reg[(*inst).dst_index];
	}
	    
	if((*inst).isSrc2Imm == true)
    {
        mem_location += (*inst).src2_index_imm;
    }
    else{
        mem_location += cpu->threads[curthread].context.reg[(*inst).src2_index_imm];
    }

		if(isLoad == true)
	{
		SIM_MemDataRead(mem_location, &cpu->threads[curthread].context.reg[(*inst).dst_index]);
	}
	else{
		SIM_MemDataWrite(mem_location, cpu->threads[curthread].context.reg[(*inst).src1_index]);
	}
	return;
}
//
void hendelInst(MT_cpu_t * cpu,int curthread, int progIndex)
{
    Instruction inst;
    SIM_MemInstRead(progIndex, &inst, curthread);
    cpu->threads[curthread].cur_inst++;

    if(inst.opcode == CMD_ADD || inst.opcode == CMD_SUB){
        int src1 = cpu->threads[curthread].context.reg[inst.src1_index];
        int src2 = cpu->threads[curthread].context.reg[inst.src2_index_imm];
        cpu->threads[curthread].context.reg[inst.dst_index] = (inst.opcode == CMD_ADD) ? (src1 + src2) : (src1 - src2);
    }

    else if(inst.opcode == CMD_ADDI || inst.opcode == CMD_SUBI){
        int src = cpu->threads[curthread].context.reg[inst.src1_index];
        int imm = inst.src2_index_imm;
        cpu->threads[curthread].context.reg[inst.dst_index] = (inst.opcode == CMD_ADDI) ? (src + imm) : (src - imm);
    }
    else if(inst.opcode == CMD_LOAD || inst.opcode == CMD_STORE){
        bool isLoad = (inst.opcode == CMD_LOAD) ? 1 : 0;
        hendelStoreAndLoad(cpu, curthread, &inst, isLoad);
        cpu->threads[curthread].active_thread = false; 
        cpu->threads[curthread].state_counter = (inst.opcode == CMD_LOAD) ? (SIM_GetLoadLat() + 1) : (SIM_GetStoreLat() + 1);
    }
    else if(inst.opcode == CMD_HALT)
    {
        cpu->threads[curthread].active_thread = false;
        cpu->threads[curthread].state_counter = -1;
        return;
    }
    else{
        return;
    }
}
void hendelSwitchingLatency(int threadnumber , MT_cpu_t* cpu)
{
	int cyclescunt = 0;
	int SimSwitchcyc = SIM_GetSwitchCycles()-1;

	while (cyclescunt < SimSwitchcyc)
	{
			threadsWakeup(cpu,threadnumber);
			cpu->cycles_count++;
			++cyclescunt;			
	}
}
//////////////////////////////////////////////////////////////////////
/* Simulates blocked MT and fine-grained MT behavior, respectively */

void CORE_BlockedMT() {

    blocked_MT = malloc(sizeof(MT_cpu_t));
	if (blocked_MT == NULL)
	{
		return;
	}
	
    int threadnumber = SIM_GetThreadsNum();
    int curthread = 0;
	int tempThread = 0;

    initCpu(blocked_MT,threadnumber);

    //run till there is no active thread
    while (IsAnyAwakeThread(blocked_MT,threadnumber) == true)
    {
        blocked_MT->cycles_count++;
        //cournt thread is ready for next inst
        threadsWakeup(blocked_MT,threadnumber);
		//if Cur thread is not active continue to the next 
        if (isAnyActiveThread(blocked_MT,threadnumber) == false)
        {
            continue;
        }
        tempThread = curthread;
        curthread = GetCurActiveThread(blocked_MT,curthread,threadnumber);
        // run switching latency if you need 
        if (tempThread != curthread)
        {
			hendelSwitchingLatency(threadnumber , blocked_MT);
            continue;
        }
		// hendel the cur Inst and continue
        hendelInst(blocked_MT,curthread, blocked_MT->threads[curthread].cur_inst);
        blocked_MT->inst_count++;
    }
	return;
}

void CORE_FinegrainedMT() {
    
	finegrained_MT = malloc(sizeof(MT_cpu_t));
	if (finegrained_MT == NULL)
	{
		return;
	}
	
	int threadnumber = SIM_GetThreadsNum();
	int curthread = 0;

    initCpu(finegrained_MT,threadnumber);

    while (IsAnyAwakeThread(finegrained_MT,threadnumber) == true)
    {
        finegrained_MT->cycles_count++;

        threadsWakeup(finegrained_MT,threadnumber);

        if (isAnyActiveThread(finegrained_MT,threadnumber) == false)
        {
            continue;
        }

        if (finegrained_MT->cycles_count > 1)
        {
            curthread = GetCurActiveThread(finegrained_MT,(curthread+1)% threadnumber,threadnumber);
        }

        hendelInst(finegrained_MT,curthread, finegrained_MT->threads[curthread].cur_inst);
        finegrained_MT->inst_count++;
    }
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////
/* Get thread register file through the context pointer */

void CORE_BlockedMT_CTX(tcontext *context, int threadid) {
    int regCounter = 0;
    while(regCounter < REGS_COUNT) {
        context[threadid].reg[regCounter] = blocked_MT->threads[threadid].context.reg[regCounter];
        ++regCounter;
    }
	return;
}

void CORE_FinegrainedMT_CTX(tcontext *context, int threadid) {
    int regCounter = 0;
    while(regCounter < REGS_COUNT){
        context[threadid].reg[regCounter]=finegrained_MT->threads[threadid].context.reg[regCounter];
        ++regCounter;
    }
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////
/* Return performance in CPI metric */

double CORE_BlockedMT_CPI() {
    double result = (double)(blocked_MT->cycles_count) / (double)(blocked_MT->inst_count);
	freeCpu(blocked_MT,SIM_GetThreadsNum());
    return result;
}

double CORE_FinegrainedMT_CPI() {
    double result = (double)(finegrained_MT->cycles_count) / (double)(finegrained_MT->inst_count);
	freeCpu(finegrained_MT,SIM_GetThreadsNum());
    return result;
}
