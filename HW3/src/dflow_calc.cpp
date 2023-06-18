/* 046267 Computer Architecture - Winter 21/22 - HW #3 */

#include "dflow_calc.h"
#include <vector>

#define INVALID -1

class NodeData{
public:
    int32_t dstIdx;
    InstInfo nodeInfo;
    int32_t weight;
    int32_t opcode;
    
    NodeData(){}
    NodeData(InstInfo info, uint32_t weight_b);
    ~NodeData(){}
};
NodeData::NodeData(InstInfo info, uint32_t weight_b){
        dstIdx = info.dstIdx;
        nodeInfo = info;
        weight = weight_b;
        opcode = info.opcode;
};

class Node{
public:
    const NodeData node_data;
    int32_t index;
    int32_t maxLen;
    int32_t numOfInsts;
    std::vector<Node *> childrens; //vector of pinters of Node
    std::vector<Node *> parents;

    Node(){}
    Node(const NodeData node_data, uint32_t index) : node_data(node_data), index(index), maxLen(-1),numOfInsts(0) {}
    Node(const NodeData node_data, uint32_t index, int32_t maxLen, int32_t numOfInsts) : node_data(node_data), index(index), maxLen(maxLen),numOfInsts(numOfInsts) {}
    ~Node(){}
};

class Graph{
public:
    int32_t numOfInsts;
    std::vector<Node *> instNodes;
    Node *src;
    Node *dst;

    Graph() : numOfInsts(0), src(new Node(NodeData(InstInfo(), 0), -1, 0, 0)),dst(new Node(NodeData(InstInfo(), 0), -2)) {}
    ~Graph(){}
};

int node_relation(Node *child, Node *parent){
    //push back: Add element at the end
    child->parents.push_back(parent);
    if(parent == NULL){
        return 0;
    }
    else{
        parent->childrens.push_back(child);
    }
    return 1;
}

void handelRelation(Node *cur_node, Node *src1_node, Node *src2_node, Graph *graph, Node *dest_node){
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

static int Max_Parent_Len(Node *node){
    int32_t max = 0;
    int32_t num_of_parents = node->parents.size();


    for (int i = 0; i<num_of_parents; i++)
    {
        Node *currParent = node->parents[i];
        if (currParent != NULL && (currParent->maxLen + currParent->node_data.weight) > max) {
            max = currParent->maxLen + currParent->node_data.weight;
        }
    }
    
    return max;
}

static int Max_Len(Graph *graph, Node *node){

    if (node->maxLen <= 0)
    {
            int32_t num_of_inst = graph->instNodes.size();
            for (int i = 0 ;i<num_of_inst ; i++)
            {
                Node *currInst = graph->instNodes[i];
                if(currInst != NULL){
                currInst->maxLen = Max_Parent_Len(currInst);
                }
            }
            graph->dst->maxLen= Max_Parent_Len(graph->dst);
    }
    else{
        return node->maxLen;
    }
    return node->maxLen;
    
}

bool check_invalid(unsigned int theInst,Graph *cur_graph){
       if (theInst > cur_graph->instNodes.size() || theInst < 0 || cur_graph == NULL)
    {
        return true;
    }
    return false; 
}

void updating_ins_indx(int *srcDepInst,Node *perents){
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

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts){   
    Graph *depen_graph = new Graph();
    Node *node_commands[32] = {NULL};   //array that contol the lasts commands
    depen_graph->numOfInsts = numOfInsts;
    for(uint32_t i = 0;i<numOfInsts; i++){
        NodeData node_data = NodeData(progTrace[i], opsLatency[progTrace[i].opcode]);
        Node *cur_node = new Node(node_data, i);

        handelRelation(cur_node,node_commands[progTrace[i].src1Idx],node_commands[progTrace[i].src2Idx],depen_graph,node_commands[progTrace[i].dstIdx]);
        node_commands[progTrace[i].dstIdx] = cur_node;
        depen_graph->instNodes.push_back(cur_node);
    }

    for(int count = 0;count<32; count++)
    {
        if (node_commands[count] != NULL && node_commands[count]->childrens.size() == 0) {
            node_relation(depen_graph->dst,node_commands[count]);
        }
    }
    return depen_graph;
}

void freeProgCtx(ProgCtx ctx){
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

int getInstDepth(ProgCtx ctx, unsigned int theInst){
    Graph *cur_graph = (Graph *) ctx;
    if (check_invalid(theInst,cur_graph)) return INVALID;
    return Max_Len(cur_graph, cur_graph->instNodes[theInst]);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst){
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
int getProgDepth(ProgCtx ctx){
    Graph *cur_graph = (Graph *) ctx;
    static int lenOfDest = Max_Len(cur_graph, cur_graph->dst);
    return lenOfDest;
}
