#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define NO_LINE -1

static int print_num;

typedef struct 
{
    uint32_t tag;
    uint32_t set;
    bool valid;
    uint32_t LRU_num;  
    bool dirtybit;
    uint32_t original_address;
} cacheLines;

typedef struct {
    uint32_t blockSize;
    uint32_t original_blockSize;
    uint32_t cacheSize;
    uint32_t assoc;
    uint32_t cyc;
    cacheLines** line;
    bool write_allocate;
    uint32_t cache_calls;
    uint32_t misses_num;
    uint32_t set_bitSize;
} cacheTable;

static double access_total_time=0;

cacheTable* cache_init(uint32_t LSize, uint32_t BSize, uint32_t LAssoc, uint32_t LCyc,bool WrAlloc)
{
    cacheTable* table = (cacheTable*)malloc(sizeof(cacheTable));
    printf("%d. im here and the first Bsize is %d\n",print_num, BSize);
    print_num++;
    table->original_blockSize = BSize;
    table->cyc = LCyc;
    table->assoc = (uint32_t)pow(2,LAssoc);
    table->blockSize = (uint32_t)pow(2,BSize);
    table->cacheSize = (uint32_t)pow(2,LSize);
    table->misses_num = 0;
    table->cache_calls = 0;
    table->write_allocate = WrAlloc;
    uint32_t num_of_blocks = (table->cacheSize)/(table->blockSize);
    table->set_bitSize = (num_of_blocks)/(table->assoc);
    table->line = (cacheLines**)malloc(num_of_blocks*sizeof(cacheLines*));
    //num of blocks = sets*assoc
    for (uint32_t i = 0; i < num_of_blocks; i++)
    {
        table->line[i] = (cacheLines *)malloc(sizeof(cacheLines));
        table->line[i]->valid = false;
        table->line[i]->dirtybit = false;
        table->line[i]->LRU_num = 0;
        table->line[i]->original_address = 0;
    }   
    return table;
}
//try to find line that match tag and set and valid. if successful retrun the cur tabel(assoc);
int find_match_tag(cacheTable* L,uint32_t tag,uint32_t set)
{
    uint32_t cur_assoc = 0;
    printf("the tag in match is %d\n",tag);
    printf("the ref in match is %d\n",set);
    //for eatch tables and cur set in table we comper the cur tag
    while (cur_assoc < L->assoc)
    {
        cacheLines* cur_line = L->line[set*L->assoc+cur_assoc];
        if(cur_line->valid != true)
        {
            return NO_LINE;
        }
        if (cur_line->tag == tag )
        {
            return cur_assoc;
        }
        cur_assoc++;        
    }
    return NO_LINE;
}
//try to find the last used LRU that match to cur set. if found set == 0 return the line.
static cacheLines* get_last_used_LRU(cacheTable* L,uint32_t set)
{
    uint32_t cur_assoc = 0;
    uint32_t min_LRU;
    printf("im here and the ref is %d\n",set);
    cacheLines* oldest_cur_line = L->line[set*L->assoc+cur_assoc];
    cacheLines* oldest_line;
    uint32_t first_min_LRU = oldest_cur_line->LRU_num;
    printf("im here and the first min LRU id %d\n",first_min_LRU);

    while (cur_assoc < L->assoc)
    {
        oldest_cur_line = L->line[set*L->assoc+cur_assoc];
        min_LRU = oldest_cur_line ->LRU_num;
        
        if (min_LRU == 0)
        {
	    printf("im here and min LRU IS 0\n");
            return oldest_cur_line;
        }
        if (min_LRU < first_min_LRU)
        {
            first_min_LRU = min_LRU;
            oldest_line = oldest_cur_line;
        }
        printf("im here and min LRU IS NOT 0\n");
        cur_assoc++;
    }
    return oldest_line;
}
//updating LRU num in blocks
void LRU_handel(cacheTable* L,uint32_t set,uint32_t tag)
{
    printf("im here AND the reciecved ref is %d\n",set);
    uint32_t accessed_index_in_set = find_match_tag(L,tag,set);
    uint32_t wantedSet = 0;
    int relevantSet = set*L->assoc;
    printf("im here AND the relevant ref is %d\n",relevantSet);
    printf("im here AND the accesesed ref is %d\n",accessed_index_in_set);
    int relevantIndexInSet = relevantSet + accessed_index_in_set;
    uint32_t prevAccess = (L->line[relevantIndexInSet])->LRU_num;
    printf("im here AND the relevant id is %d\n",relevantIndexInSet);
    (L->line[relevantIndexInSet])->LRU_num = L->assoc-1;
    while(wantedSet < L->assoc)
    {
        cacheLines* relevantEntry = L->line[relevantSet+wantedSet];
        if ((wantedSet!=accessed_index_in_set)&&(relevantEntry->LRU_num>prevAccess)&&(relevantEntry->valid))
        {
            relevantEntry->LRU_num--;
        }
        wantedSet++;
    }
}
//calculat the set according to the current address
static uint32_t cal_set(cacheTable* L,uint32_t address)
{
    int toSet = pow(2,L->original_blockSize);
    uint32_t offsetCut = address / toSet;
    uint32_t setFieldSize = L->set_bitSize-1;
    return offsetCut & setFieldSize;
}
//calculat the tag accoeding to the current address
static uint32_t cal_tag(cacheTable* L,uint32_t address)
{
    uint32_t original_set = (uint32_t)log2(L->set_bitSize);
    return address>>((L->original_blockSize)+original_set);
}
//find match valid line to set and tag and update LRU
cacheLines* get_to_cache_updating_LRU(cacheTable *L,uint32_t address)
{
    printf("%d. im here and the adress is %d\n",print_num,address);
    print_num++;
    printf("%d. im here and pre block size is %d\n",print_num, L->original_blockSize);
    print_num++;    
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address);
    int32_t cur_assoc = find_match_tag(L,tag,set);
    printf("%d. im here AND the current assoc is %d\n",print_num,cur_assoc);
    print_num++;
    if (cur_assoc!=NO_LINE)
    {
	printf("%d. im here AND im calling LRU from cache func\n" ,print_num);
	print_num++;
        LRU_handel(L,set,tag);
        return L->line[set*L->assoc+cur_assoc];
    }
    return NULL;
}
//find match valid line to set and tag if no match line return NULL
cacheLines* get_to_cache(cacheTable *L,uint32_t address)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address);
    int32_t cur_assoc = find_match_tag(L,tag,set);

    if (cur_assoc!=NO_LINE)
    {
        return L->line[set*L->assoc+cur_assoc];
    }
    return NULL;
}
//just updating new block
void update_new_block (cacheLines* new_line,uint32_t address,uint32_t tag,uint32_t set)
{
    new_line->tag = tag;
    new_line->set = set;
    new_line->valid = true;
    new_line->original_address = address;
    return;
}
//try to find the first invalid line that match to cur set. if successful return the cur line
static cacheLines* find_first_invalid(cacheTable* L,uint32_t set,bool* no_invalid)
{
    uint32_t cur_assoc = 0;
    //for eatch tables and cur set in table we take the first invalid
    while (cur_assoc < L->assoc)
    {
        cacheLines* cur_line = L->line[set*L->assoc+cur_assoc];
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
cacheLines* insert_new_block(cacheTable* L, uint32_t address,uint32_t * flip_address,bool* flip,uint32_t set,uint32_t tag)
{
    bool no_invalid = false;
    cacheLines* new_block = find_first_invalid(L,set,&no_invalid);
    //everyting valid- go to the oldest used block 
    if (no_invalid)
    {
        new_block = get_last_used_LRU(L,set);
        *flip_address = new_block->original_address;
        *flip=true;
    }
    //take the new block make valid, and updating
    update_new_block(new_block,address,tag,set);
    printf("im here AND im calling LRU from insert new block func\n");
    LRU_handel(L,set,tag);
    return new_block;
}
//hendel case of read : if secssed to read return true (hit) else return false(miss)
bool read_from_cache(cacheTable* L,uint32_t address,uint32_t * flip_address,bool* flip)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address); 
    L->cache_calls++;
   //HIT- can read from cache then return true
   cacheLines* cur_line = get_to_cache_updating_LRU(L,address);
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
bool write_to_cache(cacheTable* L,uint32_t address,uint32_t * flip_address,bool* flip)
{
    uint32_t set = cal_set(L,address);
    uint32_t tag = cal_tag(L,address);
    L->cache_calls++;
    //HIT- can write to cache then update dirty and return true
    cacheLines* cur_line = get_to_cache_updating_LRU(L,address);
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
void hendel_dirty(bool flip,bool L1_acsses,bool L2_acsses,cacheTable* L1,cacheTable* L2,uint32_t flip_address)
{
    //hendel dirty after L1 if needed
    if(L1_acsses == true && L2_acsses == false)
    {
        if(flip) 
            {
            cacheLines* L2_entry = get_to_cache(L2, flip_address);
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
            cacheLines *L1_entry = get_to_cache(L1, flip_address);
            if (L1_entry != NULL) {
            L1_entry->valid = false;
            }
        }
    }
return;
}
// hendel all the access to the cache levels: L1 ---> L2 ---> Memory 
void Cache_Update(char operation,cacheTable* L1,cacheTable* L2,uint32_t address,uint32_t MemCyc)
{
    uint32_t flip_address = 0;
    bool flip = false;
    bool L1_acsses = false;
    bool L2_acsses = false;
    //****L1****
    //go to L1 first, if seccsed to read/write - end
    access_total_time += L1->cyc;
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
    access_total_time += L2->cyc;
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
    access_total_time += MemCyc;
    //end
    return;
    
}
void free_all(cacheTable *L)
{
    uint32_t blockSize = L->blockSize;
    uint32_t cacheSize = L->cacheSize;
    uint32_t num_of_blocks = cacheSize/blockSize;

    for (size_t i = 0; i < num_of_blocks; i++)
    {
        free(L->line[i]);
    }
    free(L);
    return;
}

double cal_miss_rate(cacheTable* L)
{
    double misses = (double)(L->misses_num);
    double num_of_calls = (double)(L->cache_calls);
    return (misses / num_of_calls);
}
double cal_avg_time(cacheTable* L)
{
    double num_of_calls = (double)(L->cache_calls);
    return (access_total_time/num_of_calls);
}
