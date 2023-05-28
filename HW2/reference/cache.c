#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define EMPTY_LINE -1

static double access_overall_time=0;
static int print_num;

typedef struct
{
    uint32_t previous_add;
    uint32_t LRU_id;
    uint32_t ref;
    uint32_t cache_tag;
    bool legal;
    bool dirtybit;
} cache_rows;

typedef struct {
    uint32_t bit_extent_ref;
    bool write_assign;
    uint32_t cache_signals;
    cache_rows** row;
    uint32_t assoc;
    uint32_t previous_blockExtent;
    uint32_t blockExtent;
    uint32_t cacheExtent;
    uint32_t cycle;
    uint32_t misses_count;
}cache_board;

cache_board* cache_init(uint32_t LSize, bool write_allocation, uint32_t BSize, uint32_t LCyc, uint32_t LAssoc)
{
    cache_board* board = (cache_board*)malloc(sizeof(cache_board));
    board->cycle = LCyc;
    print_num++;
    board->previous_blockExtent = BSize;
    board->blockExtent = (uint32_t)pow(2,BSize);
    board->assoc = (uint32_t)pow(2,LAssoc);
    board->cacheExtent = (uint32_t)pow(2,LSize);
    board->cache_signals = 0;
    board->misses_count = 0;
    uint32_t blocks_count = (board->cacheExtent)/(board->blockExtent);
    board->row = (cache_rows**)malloc(blocks_count*sizeof(cache_rows*));
    board->bit_extent_ref = (blocks_count)/(board->assoc);
    board->write_assign = write_allocation;
    //count of blocks = sets*assoc
    uint32_t i = 0;
    while (i < blocks_count){
        board->row[i] = (cache_rows *)malloc(sizeof(cache_rows));
        board->row[i]->previous_add = 0;
        board->row[i]->dirtybit = false;
        board->row[i]->LRU_id = 0;
        board->row[i]->legal = false;
        i++;
    }
    return board;
}
//find line that match tag and ref and legal. if successful retrun the current board(assoc);
int match_line_to_tag(uint32_t tag, cache_board* C,uint32_t ref)
{
   //for each board and current legal in board we compare the current tag
   for (uint32_t current_assoc = 0; current_assoc < C->assoc; current_assoc++)
   {
       cache_rows* current_line = C->row[ref*C->assoc+current_assoc];
       if(current_line->legal)
       {
           if(current_line->cache_tag == tag)
           {
               return current_assoc;
           }
       }
       else{
           return EMPTY_LINE;
       }
   }
   return EMPTY_LINE;
}

//try to find the last used LRU that match to current ref. if found ref == 0 return the line.
static cache_rows* get_previous_LRU(uint32_t ref, cache_board* C)
{
    uint32_t current_assoc = 0;
    cache_rows* last_current_line = C->row[ref*C->assoc+ current_assoc];
    uint32_t  minimum_LRU;
    cache_rows* last_line;
    uint32_t min_LRU_number_1 = last_current_line->LRU_id;

    for(uint32_t current_assoc = 0;current_assoc < C->assoc;current_assoc++)
    {
        last_current_line = C->row[ref*C->assoc + current_assoc];
        minimum_LRU = last_current_line->LRU_id;
        if(minimum_LRU != 0)
        {
            if (minimum_LRU < min_LRU_number_1)
            {
                min_LRU_number_1 = minimum_LRU;
                last_line = last_current_line;
            }
        }
        else{
            return last_current_line;
        }
    }
    return last_line;
}

//updating LRU id in blocks
void LRU_update(uint32_t ref, uint32_t tag,cache_board* C)
{
    int relevant_ref = ref*C->assoc;
    uint32_t accessed_index_in_ref = match_line_to_tag(tag,C,ref);
    int relevant_id_in_ref = relevant_ref + accessed_index_in_ref;
    uint32_t previous_accessed = (C->row[relevant_id_in_ref])->LRU_id;
    (C->row[relevant_id_in_ref])->LRU_id = C->assoc-1;
    for(uint32_t wanted_ref = 0;wanted_ref < C->assoc;wanted_ref++)
    {
        cache_rows* relevant_entrance = C->row[relevant_ref+wanted_ref];
        if((wanted_ref!=accessed_index_in_ref)&&(relevant_entrance->LRU_id > previous_accessed)&&(relevant_entrance->legal))
        {
            relevant_entrance->LRU_id--;
        }
    }
}

// find match valid line to set and tag and update LRU
cache_rows* get_to_cache_updating_LRU(cache_board* C, uint32_t address){
    print_num++;
    print_num++;
    uint32_t offset = address / (pow(2, C->previous_blockExtent));
    uint32_t set = ((C->bit_extent_ref)-1) & offset;
    uint32_t tag = address >> (C->previous_blockExtent);
    tag = tag >> ((uint32_t)(log(C->bit_extent_ref) / log(2)));
    int32_t  current_assoc = match_line_to_tag(tag, C,set);
    print_num++;
    if (current_assoc != EMPTY_LINE)
    {
        print_num++;
        LRU_update(set, tag,C);
        return C->row[set*C->assoc + current_assoc];
    }

    return NULL;

}

static cache_rows* find_first_invalid(cache_board* C,uint32_t set,bool* no_invalid)
{
    for (uint32_t cur_assoc = 0; cur_assoc < C->assoc;cur_assoc++)
    {
        cache_rows* current_row = C->row[set*C->assoc+cur_assoc];
        if (current_row->legal == false)
        {
            return current_row;
        }
    }
    *no_invalid = true;
    return NULL;
}

//just updating new block
void update_new_block (cache_rows* new_line,uint32_t address,uint32_t tag,uint32_t set)
{
    new_line->cache_tag = tag;
    new_line->ref = set;
    new_line->legal = true;
    new_line->previous_add = address;
    return;
}

cache_rows* insert_new_block(cache_board* L, uint32_t address,uint32_t * flip_address,bool* flip,uint32_t ref,uint32_t tag){
    bool no_legal = false;
    cache_rows* new_block = find_first_invalid(L,ref,&no_legal);
    if (no_legal)
    {
        new_block = get_previous_LRU(ref,L);
        *flip_address = new_block->previous_add;
        *flip=true;
    }
    update_new_block(new_block, address, tag, ref);
    LRU_update(ref,tag,L);
    return new_block;
}

void Ref_Tag_Calc(cache_board* L, uint32_t address, uint32_t * ref, uint32_t * tag){
    uint32_t offset = address / (pow(2, L->previous_blockExtent));
    *ref = ((L->bit_extent_ref)-1) & offset;

    uint32_t tag_tmp = address >> (L->previous_blockExtent);
    *tag = tag_tmp >> ((uint32_t)(log(L->bit_extent_ref) / log(2)));
}


bool Read_Write_Op(char operation, cache_board* L, uint32_t address, uint32_t * flip_add, bool* flip){
    uint32_t ref, tag;
    Ref_Tag_Calc(L, address, &ref, &tag);

    L->cache_signals ++;
    cache_rows* current_line = get_to_cache_updating_LRU(L, address);
    if(operation == 'r'){
        if(current_line){
            return true;
        }
        insert_new_block(L,address,flip_add,flip,ref,tag);
        L->misses_count ++;
    }
    else if(operation == 'w'){
        if(current_line){
            current_line->dirtybit = true;
            return true;
        }
        L->misses_count ++;
        if(L->write_assign == false){
            return false;
        }
        current_line =  insert_new_block(L,address,flip_add,flip,ref,tag);
        current_line->dirtybit = true;
    }
    return false;
}

void Cache_Update(char operation, cache_board* L1, cache_board* L2, uint32_t address, uint32_t MemCyc){
    bool flip = false;
    uint32_t flip_add = 0;
    access_overall_time += (L1->cycle);
    if(Read_Write_Op(operation, L1, address, &flip_add, &flip) == true) return;
    else{
        if(flip){
            uint32_t L2_ref, L2_tag;
            Ref_Tag_Calc(L2, flip_add, &L2_ref, &L2_tag);
            int32_t current_assoc = match_line_to_tag(L2_tag, L2, L2_ref);
            cache_rows* L2_ent = (current_assoc != EMPTY_LINE)? (L2->row[L2_ref * (L2->assoc) + current_assoc]) : NULL;
            if(L2_ent && L2_ent->dirtybit){
                get_to_cache_updating_LRU(L2, flip_add);
            }
        }
    }
    access_overall_time += (L2->cycle);
    flip = false;
    flip_add = 0;
    if(Read_Write_Op(operation, L2, address, &flip_add, &flip) == true) return;
    else{
        if(flip){
            uint32_t L1_ref, L1_tag;
            Ref_Tag_Calc(L1, flip_add, &L1_ref, &L1_tag);
            int32_t current_assoc = match_line_to_tag(L1_tag, L1, L1_ref);
            cache_rows* L1_ent = (current_assoc != EMPTY_LINE)? (L1->row[L1_ref * (L1->assoc) + current_assoc]) : NULL;
            if(L1_ent != NULL){
                L1_ent->legal = false;
            }
        }
    }
    access_overall_time += MemCyc;
}


void free_all(cache_board *L)
{
    uint32_t blockExtent = L->blockExtent;
    uint32_t cacheExtent = L->cacheExtent;
    uint32_t num_of_blocks = cacheExtent/blockExtent;

    for (size_t i = 0; i < num_of_blocks; i++)
    {
        free(L->row[i]);
    }
    free(L);
    return;
}

double cal_miss_rate(cache_board* L)
{
    double misses = (double)(L->misses_count);
    double num_of_calls = (double)(L->cache_signals);
    return (misses / num_of_calls);
}
double cal_avg_time(cache_board* L)
{
    double num_of_calls = (double)(L->cache_signals);
    return (access_overall_time/num_of_calls);
}
