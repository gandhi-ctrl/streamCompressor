#include "huffmanTree.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct{
    huffman_node_t* node;
    int frequency;
} nodePointer_t;

static int _pointerCount;
static nodePointer_t _pointers[256];

static int _nodeCount;
static huffman_node_t _nodes[512];

static void _initialize(int* statistic){
    _nodeCount = 256;
    _pointerCount = 256;
    for(int n = 0; n < 256; n++){
        _nodes[n].data = n;
        _nodes[n].frequency = statistic[n];
        _nodes[n].one = NULL;
        _nodes[n].zero = NULL;

        _pointers[n].frequency = statistic[n];
        _pointers[n].node = &_nodes[n];
    }
}

static int _getMaxDept(huffman_node_t* n){
    int deptZero = 0;
    int deptOne = 0;
    if(n->zero != NULL){
        deptZero = _getMaxDept(n->zero);
    }
    if(n->one != NULL){
        deptOne = _getMaxDept(n->one);
    }
    if(deptZero > deptOne){
        return deptZero +1;
    }
    return deptOne +1;
}

static int _getMaxValue(huffman_node_t* n){
    int valZero = -1;
    int valOne = n->data;
    if(n->zero != NULL){
        valZero = _getMaxValue(n->zero);
    }
    if(n->one != NULL){
        valOne = _getMaxValue(n->one);
    }
    if(valZero > valOne){
        return valZero;
    }
    return valOne;
}

static int nodeComp(const void * a, const void * b){
    nodePointer_t* A = (nodePointer_t*)a;
    nodePointer_t* B = (nodePointer_t*)b;

    int diff = B->frequency -A->frequency;
    if(diff != 0){
        return diff;
    }

    int deptA = _getMaxDept(A->node);
    int deptB = _getMaxDept(B->node);
    diff = deptB -deptA;
    if(diff != 0){
        return diff;
    }

    int maxValA = _getMaxValue(A->node);
    int maxValB = _getMaxValue(B->node);

    return maxValA -maxValB;
}

static void _sort(){
    qsort(_pointers, _pointerCount, sizeof(nodePointer_t), nodeComp);
}

static huffman_node_t* _getNewNode(){
    huffman_node_t* ret = &_nodes[_nodeCount];
    _nodeCount++;
    return ret;
}

static void _combineLastTwo(){
    huffman_node_t* node = _getNewNode();

    nodePointer_t* A = &_pointers[_pointerCount -1];
    nodePointer_t* B = &_pointers[_pointerCount -2];

    node->data = -1;
    node->frequency = A->frequency +B->frequency;
    node->zero = B->node;
    node->one = A->node;

    B->node = node;
    B->frequency = node->frequency;
    _pointerCount--;
}

huffman_node_t* createHuffmanTree(int* statistic){
    _initialize(statistic);

    while(_pointerCount > 1){
        _sort();
        _combineLastTwo();
    }

    return _pointers[0].node;
}

static void _diveNode(huffman_node_t* node, char* str, char* cptr){
    if(node->zero != NULL){
        cptr[0] = '0';
        cptr[1] = '\0';
        _diveNode(node->zero, str, &cptr[1]);
    }
    if(node->one != NULL){
        cptr[0] = '1';
        cptr[1] = '\0';
        _diveNode(node->one, str, &cptr[1]);
    }
    if(node->data >= 0){
        fprintf(stderr, "%s -> 0x%02X", str, node->data);
        if(isprint(node->data) != 0){
            fprintf(stderr, " '%c'", node->data);
        }
        fprintf(stderr, "\n");
    }
}

void listHuffmanCodes(huffman_node_t* root){
    char tmp[64];
    tmp[0] = '\0';
    _diveNode(root, tmp, tmp);
}