#ifndef _HUFFMAN_TREE_H_
#define _HUFFMAN_TREE_H_

#include <stdint.h>

typedef struct huffman_node_t huffman_node_t;

struct huffman_node_t{
    int data;
    int frequency;
    huffman_node_t* zero;
    huffman_node_t* one;
};

huffman_node_t* createHuffmanTree(int* statistic);
void listHuffmanCodes(huffman_node_t* root);

#endif // _HUFFMAN_TREE_H_
