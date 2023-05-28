#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define EMPTY_LINE -1

static double access_overall_time=0;

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
uint32_t match_line_to_tag(uint32_t tag, cache_board* C,uint32_t ref)
{
   //for each board and current legal in board we compare the current tag
   for (uint32_t current_assoc = 0; current_assoc < C->assoc; current_assoc++)
   {
       cache_rows* current_line = C->row[ref*(C->assoc)+current_assoc];
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
    cache_rows* last_current_line = C->row[ref*(C->assoc)+ current_assoc];
    uint32_t  minimum_LRU;
    cache_rows* last_line;
    uint32_t min_LRU_number_1 = last_current_line->LRU_id;

    for(uint32_t current_assoc = 0;current_assoc < C->assoc;current_assoc++)
    {
        last_current_line = C->row[ref*(C->assoc) + current_assoc];
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
    int relevant_ref = ref*(C->assoc);
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
        wanted_ref++;
    }
}

cache_rows* get_to_cache_updating_LRU(cache_board* L, uint32_t address){
    return NULL;
}
cache_rows* insert_new_block(cache_board* L, uint32_t address,uint32_t * flip_address,bool* flip,uint32_t ref,uint32_t tag){
    return NULL;
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
            Ref_Tag_Calc(L2, address, &L2_ref, &L2_tag);
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
            Ref_Tag_Calc(L1, address, &L1_ref, &L1_tag);
            int32_t current_assoc = match_line_to_tag(L1_tag, L1, L1_ref);
            cache_rows* L1_ent = (current_assoc != EMPTY_LINE)? (L1->row[L1_ref * (L1->assoc) + current_assoc]) : NULL;
            if(L1_ent != NULL){
                L1_ent->legal = false;
            }
        }
    }
    access_overall_time += MemCyc;
}


/*
//calculat the set according to the current address
static uint32_t cal_set(cache_board* L,uint32_t address)
{
    int toSet = pow(2,L->original_blockExtent);
    uint32_t offsetCut = address / toSet;
    uint32_t setFieldSize = L->set_bitSize-1;
    return offsetCut & setFieldSize;
}
//calculat the tag accoeding to the current address
static uint32_t cal_tag(cache_board* L,uint32_t address)
{
    uint32_t original_set = (uint32_t)log2(L->set_bitSize);
    return address>>((L->original_blockExtent)+original_set);
}
//find match valid line to set and tag and update LRU
cache_rows* get_to_cache_updating_LRU(cache_board *L,uint32_t address)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address);
    int32_t cur_assoc = match_line_to_tag(L,tag,set);

    if (cur_assoc!=NO_LINE)
    {
        LRU_update(set,tag,L);
        return L->row[set*L->assoc+cur_assoc];
    }
    return NULL;
}
//find match valid line to set and tag if no match line return NULL
cache_rows* get_to_cache(cache_board *L,uint32_t address)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address);
    int32_t cur_assoc = match_line_to_tag(L,tag,set);

    if (cur_assoc!=NO_LINE)
    {
        return L->row[set*L->assoc+cur_assoc];
    }
    return NULL;
}
//just updating new block
void update_new_block (cache_rows* new_line,uint32_t address,uint32_t tag,uint32_t set)
{
    new_line->tag = tag;
    new_line->set = set;
    new_line->valid = true;
    new_line->original_address = address;
    return;
}
//try to find the first invalid line that match to cur set. if successful return the cur line
static cache_rows* find_first_invalid(cache_board* L,uint32_t set,bool* no_invalid)
{
    uint32_t cur_assoc = 0;
    //for eatch tables and cur set in table we take the first invalid
    while (cur_assoc < L->assoc)
    {
        cache_rows* cur_line = L->line[set*L->assoc+cur_assoc];
        if(cur_line->valid == false)
        {
            return cur_line;
        }
        cur_assoc++; 
    }
    *no_invalid = true;
    return NULL;
}
//insert new block in the cache: first- try to find invalid and make it valid (match to set) if everything valid- go to the last LRU
cache_rows* insert_new_block(cache_board* L, uint32_t address,uint32_t * flip_address,bool* flip,uint32_t set,uint32_t tag)
{
    bool no_invalid = false;
    cache_rows* new_block = find_first_invalid(L,set,&no_invalid);
    //everyting valid- go to the oldest used block 
    if (no_invalid)
    {
        new_block = get_previous_LRU(L,set);
        *flip_address = new_block->original_address;
        *flip=true;
    }
    //take the new block make valid, and updating
    update_new_block(new_block,address,tag,set);
    LRU_update(set,tag,L);
    return new_block;
}
//hendel case of read : if secssed to read return true (hit) else return false(miss)
bool read_from_cache(cache_board* L,uint32_t address,uint32_t * flip_address,bool* flip)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address); 
    L->cache_calls++;
   //HIT- can read from cache then return true
   cache_rows* cur_line = get_to_cache_updating_LRU(L,address);
    if(cur_line!=NULL)
    {
        return true;
    }
   //MISS- cant read from cache, updating new block 
    insert_new_block(L,address,flip_address,flip,set,tag);  
    L->misses_num++;
    return false;
}
//hendel case of write : if secssed to write return true (hit) else return false(miss)
bool write_to_cache(cache_board* L,uint32_t address,uint32_t * flip_address,bool* flip)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address);
    L->cache_calls++;
    //HIT- can write to cache then update dirty and return true
    cache_rows* cur_line = get_to_cache_updating_LRU(L,address);
    if(cur_line!=NULL)
    {
        cur_line->dirtybit=true;
        return true;
    }
    if(cur_line == NULL) //MISS- cant write to cache, hendel case of write and no write allocate
    {
        L->misses_num++;
        if(!L->write_allocate)
        {
            return false;
        }
        cur_line = insert_new_block(L,address,flip_address,flip,set,tag);
    }
    cur_line->dirtybit=true;           
    return false;
}
// hendel dirty bit flow
void hendel_dirty(bool flip,bool L1_acsses,bool L2_acsses,cache_board* L1,cache_board* L2,uint32_t flip_address)
{
    //hendel dirty after L1 if needed
    if(L1_acsses == true && L2_acsses == false)
    {
        if(flip) 
            {
            cache_rows* L2_entry = get_to_cache(L2, flip_address);
            if(L2_entry->dirtybit)
            {
            get_to_cache_updating_LRU(L2, flip_address);
            }
        }
    }
    //hendel dirty after L2 if needed
    if(L1_acsses == true && L2_acsses == true)
    {
        if(flip) 
        {
            cache_rows *L1_entry = get_to_cache(L1, flip_address);
            if (L1_entry != NULL) {
            L1_entry->valid = false;
            }
        }
    }
return;
}
// hendel all the access to the cache levels: L1 ---> L2 ---> Memory 
void Cache_Update(char operation,cache_board* L1,cache_board* L2,uint32_t address,uint32_t Memcycle)
{
    uint32_t flip_address = 0;
    bool flip = false;
    bool L1_acsses = false;
    bool L2_acsses = false;
    //****L1****
    //go to L1 first, if seccsed to read/write - end
    access_total_time += L1->cycle;
    L1_acsses = true;

    if(operation == 'r')
    {
        if(read_from_cache(L1,address,&flip_address,&flip))
        return;
    }
    else
    {
        if(write_to_cache(L1,address,&flip_address,&flip))
        return;
    }  
    hendel_dirty(flip,L1_acsses,L2_acsses,L1,L2,flip_address);
    //****L2****
    //go to L2 if seccsed to read/write - end
    access_total_time += L2->cycle;
    L2_acsses = true;
    flip_address = 0;
    flip = false;
    if (operation == 'r')
    {
        if (read_from_cache(L2,address,&flip_address,&flip))
        return;
    }
    else
    {
        if(write_to_cache(L2,address,&flip_address,&flip))
        return;
    }
    hendel_dirty(flip,L1_acsses,L2_acsses,L1,L2,flip_address);
    //****Memory****
    //go to the memory and this is the end
    access_total_time += Memcycle;
    //end
    return;
    
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
    double misses = (double)(L->misses_num);
    double num_of_calls = (double)(L->cache_calls);
    return (misses / num_of_calls);
}
double cal_avg_time(cache_board* L)
{
    double num_of_calls = (double)(L->cache_calls);
    return (access_total_time/num_of_calls);
}

 */
