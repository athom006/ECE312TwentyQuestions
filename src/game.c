#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "lab5.h"

extern Node *g_root;
extern EditStack g_undo;
extern EditStack g_redo;
extern Hash g_index;

/* TODO 31: Implement play_game
 * Main game loop using iterative traversal with a stack
 * 
 * Key requirements:
 * - Use FrameStack (NO recursion!)
 * - Push frames for each decision point
 * - Track parent and answer for learning
 * 
 * Steps:
 * 1. Initialize and display game UI
 * 2. Initialize FrameStack
 * 3. Push root frame with answeredYes = -1
 * 4. Set parent = NULL, parentAnswer = -1
 * 5. While stack not empty:
 *    a. Pop current frame
 *    b. If current node is a question:
 *       - Display question and get user's answer (y/n)
 *       - Set parent = current node
 *       - Set parentAnswer = answer
 *       - Push appropriate child (yes or no) onto stack
 *    c. If current node is a leaf (animal):
 *       - Ask "Is it a [animal]?"
 *       - If correct: celebrate and break
 *       - If wrong: LEARNING PHASE
 *         i. Get correct animal name from user
 *         ii. Get distinguishing question
 *         iii. Get answer for new animal (y/n for the question)
 *         iv. Create new question node and new animal node
 *         v. Link them: if newAnswer is yes, newQuestion->yes = newAnimal
 *         vi. Update parent pointer (or g_root if parent is NULL)
 *         vii. Create Edit record and push to g_undo
 *         viii. Clear g_redo stack
 *         ix. Update g_index with canonicalized question
 * 6. Free stack
 */
void play_game() {
    clear();
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(0, 0, "%-80s", " Playing 20 Questions");
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    mvprintw(2, 2, "Think of an animal, and I'll try to guess it!");
    mvprintw(3, 2, "Press any key to start...");
    refresh();
    getch();
    
    // TODO: Implement the game loop
    // Initialize FrameStack
    // Push root
    // Loop until stack empty or guess is correct
    // Handle question nodes and leaf nodes differently
    
    FrameStack stack;
    fs_init(&stack);
    fs_push(&stack, g_root, -1);
    Node *parent = NULL;
    int parentAnswer = -1;

    while(!fs_empty(&stack)){
        
        for (int i = 5; i < 12; i++) {
            mvprintw(i, 2, "%-76s", ""); // Clear area before printing new content
        }
        refresh();

        Frame current = fs_pop(&stack);
        if(current.node->isQuestion==1){ //question node
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "%s (y/n)? ", current.node->text);
            int answer = get_yes_no(5, 2, prompt);
            parent = current.node;
            parentAnswer = answer;
            // Push next frame based on answer
            if(answer == 1){ 
                fs_push(&stack, current.node->yes, answer);
            } else {
                fs_push(&stack, current.node->no, answer);
            }
        }
        if(current.node->isQuestion==0){//leaf node
            char prompt[256];
            snprintf(prompt, sizeof(prompt), "Is it a %s (y/n)? ", current.node->text);
            int correct = get_yes_no(5, 2, prompt);
            if(correct == 1){ //guessed correctly
                mvprintw(7, 2, "Yay! I guessed it right!");
                refresh();
                getch();
                break;
            } else { //guessed wrong, enter learning phase
                // Learning phase
                char *newAnimal = get_input(7, 2, "What animal were you thinking of? ");
                Node *newAnimalNode = create_animal_node(newAnimal);

                char *newQuestion = get_input(9, 2, "Please provide a question that distinguishes your animal from mine: ");
                Node *newQuestionNode = create_question_node(newQuestion);
                
                int newAnswer = get_yes_no(11, 2, "For your animal, is the answer to your question 'yes' or 'no' (y/n)? ");
                // Create new nodes
                

                if(newAnswer == 1){
                    newQuestionNode->yes = newAnimalNode;
                    newQuestionNode->no = current.node;
                } else {
                    newQuestionNode->no = newAnimalNode;
                    newQuestionNode->yes = current.node;
                }
            
                // Update parent pointer
                if(parent == NULL){
                    g_root = newQuestionNode;
                } else if(parentAnswer == 1){
                    parent->yes = newQuestionNode;
                } else {
                    parent->no = newQuestionNode;
                }
            
                Edit edit; // Create edit record
                edit.type = EDIT_INSERT_SPLIT;
                edit.parent = parent;
                edit.wasYesChild = parentAnswer;
                edit.oldLeaf = current.node;
                edit.newQuestion = newQuestionNode;
                edit.newLeaf = newAnimalNode;
                es_push(&g_undo, edit);
                es_clear(&g_redo);
                char *canonicalQ = canonicalize(newQuestion);
                h_put(&g_index, canonicalQ, count_nodes(newAnimalNode)); // Using count_nodes as a dummy animal ID
                free(canonicalQ);
                break;
            }
        }

        refresh(); // Update display after each interaction

    }
    
    // TODO: Your implementation here
    
    fs_free(&stack);
}

/* TODO 32: Implement undo_last_edit
 * Undo the most recent tree modification
 * 
 * Steps:
 * 1. Check if g_undo stack is empty, return 0 if so
 * 2. Pop edit from g_undo
 * 3. Restore the tree structure:
 *    - If edit.parent is NULL:
 *      - Set g_root = edit.oldLeaf
 *    - Else if edit.wasYesChild:
 *      - Set edit.parent->yes = edit.oldLeaf
 *    - Else:
 *      - Set edit.parent->no = edit.oldLeaf
 * 4. Push edit to g_redo stack
 * 5. Return 1
 * 
 * Note: We don't free newQuestion/newLeaf because they might be redone
 */
int undo_last_edit() {
    // TODO: Implement this function

    if(es_empty(&g_undo)){
        return 0;
    }

    Edit edit = es_pop(&g_undo); // Pop the last edit
    if(edit.parent == NULL){ // If parent is NULL, we are undoing the root
        g_root = edit.oldLeaf;
    } else if(edit.wasYesChild == 1){ // If it was a yes child, restore the yes pointer
        edit.parent->yes = edit.oldLeaf;
    } else { // If it was a no child, restore the no pointer
        edit.parent->no = edit.oldLeaf;
    }
    es_push(&g_redo, edit); // Push the edit to the redo stack

    return 1;
}

/* TODO 33: Implement redo_last_edit
 * Redo a previously undone edit
 * 
 * Steps:
 * 1. Check if g_redo stack is empty, return 0 if so
 * 2. Pop edit from g_redo
 * 3. Reapply the tree modification:
 *    - If edit.parent is NULL:
 *      - Set g_root = edit.newQuestion
 *    - Else if edit.wasYesChild:
 *      - Set edit.parent->yes = edit.newQuestion
 *    - Else:
 *      - Set edit.parent->no = edit.newQuestion
 * 4. Push edit back to g_undo stack
 * 5. Return 1
 */
int redo_last_edit() {
    // TODO: Implement this function
    if(es_empty(&g_redo)){
        return 0;
    }
    
    Edit edit = es_pop(&g_redo);

    if(edit.parent == NULL){ // If parent is NULL, we are redoing the root
        g_root = edit.newQuestion;
    } else if(edit.wasYesChild == 1){ // If it was a yes child, restore the yes pointer
        edit.parent->yes = edit.newQuestion;
    } else { // If it was a no child, restore the no pointer
        edit.parent->no = edit.newQuestion;
    }
    es_push(&g_undo, edit);// Push the edit back to the undo stack

    return 1;
}
