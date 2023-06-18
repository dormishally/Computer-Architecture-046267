/* 046267 Computer Architecture - Winter 21/22 - HW #3 */

#include "dflow_calc.h"
#include <vector>

#define INVALID -1

class NodeData 
{
public:
    int32_t dstIdx;
    InstInfo nodeInfo;
    int32_t weight;
    int32_t opcode;
    NodeData(InstInfo info, uint32_t weight) : dstIdx(info.dstIdx), nodeInfo(info), weight(weight),opcode(info.opcode) {}
};

class Node 
{
public:
    const NodeData node_data;
    int32_t index;
    int32_t maxLen;
    int32_t numOfInsts;
    std::vector<Node *> childrens; //vector of pinters of Node
    std::vector<Node *> parents;
    Node(const NodeData node_data, uint32_t index) : node_data(node_data), index(index), maxLen(-1),numOfInsts(0) {}

};

class Graph 
{
public:
    int32_t numOfInsts;
    std::vector<Node *> instNodes;
    Node *src;
    Node *dst;

    Graph() : numOfInsts(0), src(new Node(NodeData(InstInfo(), 0), -1)),dst(new Node(NodeData(InstInfo(), 0), -2)) 
    {
        src->maxLen = 0;
        src->index = 0;
    }
};

int node_relation(Node *child, Node *parent) 
{
    //push back: Add element at the end
    child->parents.push_back(parent);
    if(parent == NULL)
    {
        return 0;
    }
    else 
    {
        parent->childrens.push_back(child);
    }
    return 1;
}

void handelRelation(Node *cur_node, Node *src1_node, Node *src2_node, Graph *graph, Node *dest_node)
{
    if ((node_relation(cur_node, src1_node) + node_relation(cur_node, src2_node)) == 0)
    {
        node_relation(cur_node, graph->src);
    }
    if(dest_node != NULL)
    {
        if (dest_node->childrens.size() == 0)
        {
            node_relation(graph->dst,dest_node);
            return; 
        }
        return;            
    }
    return;
}

static int Max_Parent_Len(Node *node)
{
    int32_t max = 0;
    int32_t num_of_parents = node->parents.size();
    int i = 0;

    while (i<num_of_parents)
    {
        Node *currParent = node->parents[i];
        if (currParent != NULL && (currParent->maxLen + currParent->node_data.weight) > max) {
            max = currParent->maxLen + currParent->node_data.weight;
        }
        i++;
    }
    
    return max;
}

static int Max_Len(Graph *graph, Node *node) 
{
    int i=0;
    if (node->maxLen > 0)
    {
        return node->maxLen;
    } 
    else 
    {
        int32_t num_of_inst = graph->instNodes.size();
        while (i<num_of_inst) 
        {
            Node *currInst = graph->instNodes[i];
            if(currInst != NULL){
            currInst->maxLen = Max_Parent_Len(currInst);
            }
            i++;
        }
        graph->dst->maxLen= Max_Parent_Len(graph->dst);
    }
    return node->maxLen;
}

bool check_invalid(unsigned int theInst,Graph *cur_graph)
{
       if (theInst > cur_graph->instNodes.size() || theInst < 0 || cur_graph == NULL)
    {
        return true;
    }
    return false; 
}

void updating_ins_indx(int *srcDepInst,Node *perents)
{
    if(perents != NULL)
    {
        *srcDepInst = perents->index;       
    }
    else
    {
        *srcDepInst = INVALID;
    }
    return;
}

/** analyzeProg: Analyze given program and save results
    \param[in] opsLatency An array of MAX_OPS values of functional unit latency for each opcode
               (some entries may be unused - in that case their value would be 0)
    \param[in] progTrace An array of instructions information from execution trace of a program
    \param[in] numOfInsts The number of instructions in progTrace[]
    \returns Analysis context that may be queried using the following query functions or PROG_CTX_NULL on failure */

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
 {   
    Graph *depen_graph = new Graph();
    Node *node_commands[32] = {NULL};   //array that contol the lasts commands
    depen_graph->numOfInsts = numOfInsts;
    uint32_t i = 0;
    while(i<numOfInsts){
        NodeData node_data = NodeData(progTrace[i], opsLatency[progTrace[i].opcode]);
        Node *cur_node = new Node(node_data, i);

        handelRelation(cur_node,node_commands[progTrace[i].src1Idx],node_commands[progTrace[i].src2Idx],depen_graph,node_commands[progTrace[i].dstIdx]);
        node_commands[progTrace[i].dstIdx] = cur_node;
        depen_graph->instNodes.push_back(cur_node);
        ++i;
    }

    int count = 0;
    while(count<32)
    {
        if (node_commands[count] != NULL && node_commands[count]->childrens.size() == 0) {
            node_relation(depen_graph->dst,node_commands[count]);
        }
        ++count;
    }
    return depen_graph;
}

/** freeProgCtx: Free the resources associated with given program context
    \param[in] ctx The program context to free
*/
void freeProgCtx(ProgCtx ctx) 
{
    Graph *cur_graph = (Graph *) ctx;
    int32_t numOfInst = cur_graph->numOfInsts;
    int32_t i = 0;
    while(i<numOfInst){
        Node *cur_node = cur_graph->instNodes[i];
        free(cur_node);
        ++i;
    }
    free(cur_graph);
}

/** getInstDepth: Get the dataflow dependency depth in clock cycles
    Instruction that are direct decendents to the entry node (depend only on Entry) should return 0
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program trace to query (the index in given progTrace[])
    \returns >= 0 The dependency depth, <0 for invalid instruction index for this program context
*/
int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    Graph *cur_graph = (Graph *) ctx;
    if (check_invalid(theInst,cur_graph)) return INVALID;
    return Max_Len(cur_graph, cur_graph->instNodes[theInst]);
}

/** getInstDeps: Get the instructions that a given instruction depends upon
    \param[in] ctx The program context as returned from analyzeProg()
    \param[in] theInst The index of the instruction of the program trace to query (the index in given progTrace[])
    \param[out] src1DepInst Returned index of the instruction that src1 depends upon (-1 if depends on "entry")
    \param[out] src2DepInst Returned index of the instruction that src2 depends upon (-1 if depends on "entry")
    \returns 0 for success, <0 for error (e.g., invalid instruction index)
*/
int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    Graph *cur_graph = (Graph *) ctx;
    if (check_invalid(theInst,cur_graph)) 
    {
        return INVALID;
    }
    Node *cur_node = cur_graph->instNodes[theInst];
    if (cur_node != NULL) {
        updating_ins_indx (src1DepInst,cur_node->parents[0]);
        updating_ins_indx (src2DepInst,cur_node->parents[1]);
        return 0;
    }

    return INVALID;
}

/** getProgDepth: Get the longest execution path of this program (from Entry to Exit)
    \param[in] ctx The program context as returned from analyzeProg()
    \returns The longest execution path duration in clock cycles
*/
int getProgDepth(ProgCtx ctx) {
    Graph *cur_graph = (Graph *) ctx;
    static int lenOfDest = Max_Len(cur_graph, cur_graph->dst);
    return lenOfDest;
}
