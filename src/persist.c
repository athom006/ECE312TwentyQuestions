#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lab5.h"

extern Node *g_root;

#define MAGIC 0x41544C35  /* "ATL5" */
#define VERSION 1

typedef struct {
    Node *node;
    int id;
} NodeMapping;

/* TODO 27: Implement save_tree
 * Save the tree to a binary file using BFS traversal
 * 
 * Binary format:
 * - Header: magic (4 bytes), version (4 bytes), nodeCount (4 bytes)
 * - For each node in BFS order:
 *   - isQuestion (1 byte)
 *   - textLen (4 bytes)
 *   - text (textLen bytes, no null terminator)
 *   - yesId (4 bytes, -1 if NULL)
 *   - noId (4 bytes, -1 if NULL)
 * 
 * Steps:
 * 1. Return 0 if g_root is NULL
 * 2. Open file for writing binary ("wb")
 * 3. Initialize queue and NodeMapping array
 * 4. Use BFS to assign IDs to all nodes:
 *    - Enqueue root with id=0
 *    - Store mapping[0] = {g_root, 0}
 *    - While queue not empty:
 *      - Dequeue node and id
 *      - If node has yes child: add to mappings, enqueue with new id
 *      - If node has no child: add to mappings, enqueue with new id
 * 5. Write header (magic, version, nodeCount)
 * 6. For each node in mapping order:
 *    - Write isQuestion, textLen, text bytes
 *    - Find yes child's id in mappings (or -1)
 *    - Find no child's id in mappings (or -1)
 *    - Write yesId, noId
 * 7. Clean up and return 1 on success
 */
int save_tree(const char *filename) {
    // TODO: Implement this function
    // This is complex - break it into smaller steps
    // You'll need to use the Queue functions you implemented

    if(g_root == NULL){ //Return 0 if g_root is NULL
        return 0;
    }

    FILE *file = fopen(filename, "wb"); //open file for writing binary

    if(file == NULL){ //return if failed
         return 0;
    }

    Queue q; //Initialize queue and NodeMapping array
    q_init(&q);
    NodeMapping *mappings = NULL;
    int mappingCapacity = 16;
    int mappingSize = 0;
    mappings = (NodeMapping *)malloc(mappingCapacity * sizeof(NodeMapping));
    if(mappings == NULL){
        return 0;
    }

    q_enqueue(&q, g_root, 0); //Use BFS to assign IDs to all nodes
    mappings[0].node = g_root;
    mappings[0].id = 0;
    mappingSize++;
    while(!q_empty(&q)){
        Node *node;
        int id;
        q_dequeue(&q, &node, &id);
        if(node->yes != NULL){
            if(mappingSize >= mappingCapacity){
                mappingCapacity *= 2;
                mappings = (NodeMapping *)realloc(mappings, mappingCapacity * sizeof(NodeMapping));
            }
            mappings[mappingSize].node = node->yes;
            mappings[mappingSize].id = mappingSize;
            q_enqueue(&q, node->yes, mappingSize);
            mappingSize++;
        }
        if(node->no != NULL){
            if(mappingSize >= mappingCapacity){
                mappingCapacity *= 2;
                mappings = (NodeMapping *)realloc(mappings, mappingCapacity * sizeof(NodeMapping));
            }
            mappings[mappingSize].node = node->no;
            mappings[mappingSize].id = mappingSize;
            q_enqueue(&q, node->no, mappingSize);
            mappingSize++;
        }
    }
    // Write header
    uint32_t magic = MAGIC;
    uint32_t version = VERSION;
    uint32_t nodeCount = mappingSize;
    fwrite(&magic, sizeof(uint32_t), 1, file);
    fwrite(&version, sizeof(uint32_t), 1, file);
    fwrite(&nodeCount, sizeof(uint32_t), 1, file);
    // Write nodes
    for(int i = 0; i < mappingSize; i++){
        Node *node = mappings[i].node;
        uint8_t isQuestion = node->isQuestion;
        uint32_t textLen = strlen(node->text);
        fwrite(&isQuestion, sizeof(uint8_t), 1, file);
        fwrite(&textLen, sizeof(uint32_t), 1, file);
        fwrite(node->text, sizeof(char), textLen, file);
        int32_t yesId = -1;
        int32_t noId = -1;
        for(int j = 0; j < mappingSize; j++){
            if(mappings[j].node == node->yes){
                yesId = mappings[j].id;
            }
            if(mappings[j].node == node->no){
                noId = mappings[j].id;
            }
        }
        fwrite(&yesId, sizeof(int32_t), 1, file);
        fwrite(&noId, sizeof(int32_t), 1, file);
    }
    free(mappings); //Clean up
    q_free(&q);
    fclose(file);
    return 1;
}
    


/* TODO 28: Implement load_tree
 * Load a tree from a binary file and reconstruct the structure
 * 
 * Steps:
 * 1. Open file for reading binary ("rb")
 * 2. Read and validate header (magic, version, count)
 * 3. Allocate arrays for nodes and child IDs:
 *    - Node **nodes = calloc(count, sizeof(Node*))
 *    - int32_t *yesIds = calloc(count, sizeof(int32_t))
 *    - int32_t *noIds = calloc(count, sizeof(int32_t))
 * 4. Read each node:
 *    - Read isQuestion, textLen
 *    - Validate textLen (e.g., < 10000)
 *    - Allocate and read text string (add null terminator!)
 *    - Read yesId, noId
 *    - Validate IDs are in range [-1, count)
 *    - Create Node and store in nodes[i]
 * 5. Link nodes using stored IDs:
 *    - For each node i:
 *      - If yesIds[i] >= 0: nodes[i]->yes = nodes[yesIds[i]]
 *      - If noIds[i] >= 0: nodes[i]->no = nodes[noIds[i]]
 * 6. Free old g_root if not NULL
 * 7. Set g_root = nodes[0]
 * 8. Clean up temporary arrays
 * 9. Return 1 on success
 * 
 * Error handling:
 * - If any read fails or validation fails, goto load_error
 * - In load_error: free all allocated memory and return 0
 */
int load_tree(const char *filename) {
    // TODO: Implement this function
    // This is the most complex function in the lab
    // Take it step by step and test incrementally
    if(filename == NULL){
        return 0;
    }
    FILE *file = fopen(filename, "rb");
    if(file == NULL){
    return 0;
    }

    uint32_t magic, version, count;
    if(fread(&magic, sizeof(uint32_t), 1, file) != 1 || magic != MAGIC){
        fclose(file);
        return 0;   
    }
    if(fread(&version, sizeof(uint32_t), 1, file) != 1 || version != VERSION){
        fclose(file);
        return 0;   
    }
    if(fread(&count, sizeof(uint32_t), 1, file) != 1 || count == 0){
        fclose(file);
        return 0;
    }

    Node **nodes = (Node **)calloc(count, sizeof(Node*));
    int32_t *yesIds = (int32_t *)calloc(count, sizeof(int32_t));
    int32_t *noIds = (int32_t *)calloc(count, sizeof(int32_t));
    if(nodes == NULL || yesIds == NULL || noIds == NULL){
        free(nodes);
        free(yesIds);
        free(noIds);
        fclose(file);
        return 0;
    }

    for(uint32_t i = 0; i < count; i++){
        uint8_t isQuestion;
        uint32_t textLen;
    
        if(fread(&isQuestion, sizeof(uint8_t), 1, file) != 1){
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }
        if(fread(&textLen, sizeof(uint32_t), 1, file) != 1 || textLen == 0 || textLen >= 10000){
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }
        char *text = (char *)malloc(textLen + 1);
        if(text == NULL){
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }
        if(fread(text, sizeof(char), textLen, file) != textLen){
            free(text);
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }
        text[textLen] = '\0'; // Null-terminate the string
        if(fread(&yesIds[i], sizeof(int32_t), 1, file) != 1 || yesIds[i] < -1 || yesIds[i] >= (int32_t)count){
            free(text);
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }
        if(fread(&noIds[i], sizeof(int32_t), 1, file) != 1 || noIds[i] < -1 || noIds[i] >= (int32_t)count){
            free(text);
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }

        nodes[i] = (Node *)malloc(sizeof(Node));
        if(nodes[i] == NULL){
            free(text);
            free(nodes);
            free(yesIds);
            free(noIds);
            fclose(file);
            return 0;
        }

        nodes[i]->isQuestion = isQuestion;
        nodes[i]->text = text;
        nodes[i]->yes = NULL;
        nodes[i]->no = NULL;
    }


    for(uint32_t i = 0; i < count; i++){
        if(yesIds[i] >= 0){
            nodes[i]->yes = nodes[yesIds[i]];
        }
        if(noIds[i] >= 0){
            nodes[i]->no = nodes[noIds[i]];
        }
    }

    if(g_root != NULL){
        free_tree(g_root);
    }
    g_root = nodes[0];

    free(yesIds);
    free(noIds);
    free(nodes);
    fclose(file);
    return 1;  

    

}
