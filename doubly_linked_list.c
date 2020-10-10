/* 
    doubly_linked_list.c - a doubly linked list implementation specifically for messages
    based on https://github.com/the-c0d3r/data-structures-c/blob/master/src/doublelinkedlist.c
*/


typedef struct node_t {
    struct Message* data;
    //int value
    struct node_t* next;
    struct node_t* prev;
} node_t;

typedef struct {
    struct node_t* head;
    struct node_t* last;
    int size;
} double_linkedlist_t;

/**
 * @brief creates a new node
 * @param value: the value to write to
 * @return: ptr to newly created node || null
 */
node_t* create_node(Message* data) {
    node_t* node = malloc(1 * sizeof(node_t));
    if (node) {
        node->data = data;
        node->next = NULL;
        node->prev = NULL;
        return node;
    } else {
        printf("Error allocating memory\n");
        return NULL;
    }
}

/**
 * @brief destroy the node properly
 * @param node: the node to delete
 */
void destroy_node(node_t* node) {
    free(node);
}

/**
 * @brief function to create new double linkedlist
 * @return: ptr to newly created double linkedlist || null
 */
double_linkedlist_t* create_double_linkedlist() {
    double_linkedlist_t* dlinkedlist = malloc(1 * sizeof(double_linkedlist_t));
    if (!dlinkedlist) {
        printf("Error allocating memory\n");
        return NULL;
    }
    dlinkedlist->size = 0;
    return dlinkedlist;
}

/**
 * @brief function to append node to list
 * @param dlinkedlist: double linked list
 * @param node: the node to insert
 * @return: 0 on success | 1 on failure
 */
int list_append(double_linkedlist_t* dlinkedlist, node_t* node) {
    if (!dlinkedlist || !node) return 1;

    // empty linkedlist
    if (dlinkedlist->size == 0) {
        dlinkedlist->head = node;
        dlinkedlist->last = node;
        dlinkedlist->size++;
    } else {
        dlinkedlist->last->next = node;
        node->prev = dlinkedlist->last;
        dlinkedlist->last = node;
        dlinkedlist->size++;
    }

    return 0;
}

/**
 * @brief function to remove the node from dlinkedlist
 * @param dlinkedlist: the double linkedlist
 * @param node: the node to remove
 * @return: 0 on success | 1 on failure
 */
int list_delete(double_linkedlist_t* dlinkedlist, node_t* node) {
    if (!dlinkedlist || !node || dlinkedlist->size == 0) return 1;

    // case when node is the only node
    if (dlinkedlist->size == 1) {
        destroy_node(node);
        dlinkedlist->head = NULL;
        dlinkedlist->last = NULL;
        dlinkedlist->size--;
        return 0;
    }

    node_t* pointer = dlinkedlist->head;
    while (pointer) {
        if (pointer == node) break;
        pointer = pointer->next;
    }
    // if node is not found return 1
    if (!pointer) return 1;

    // case when last node
    if (node == dlinkedlist->last) {
        dlinkedlist->last = pointer->prev;
        pointer->prev->next = pointer->next;
    } else if (node == dlinkedlist->head) {
        dlinkedlist->head = pointer->next;
        pointer->next->prev = pointer->prev;
    } else {
        pointer->prev->next = pointer->next;
        pointer->next->prev = pointer->prev;
    }

    destroy_node(pointer);
    dlinkedlist->size--;
    return 0;
}

/**
 * @brief function to properly destroy the dlinkedlist
 * @param dlinkedlist: the dlinkedlist to destroy
 * @return: 0 on success || 1 on failure
 */
int destroy(double_linkedlist_t* dlinkedlist) {
    if (!dlinkedlist) return 1;

    node_t* pointer = dlinkedlist->head;
    node_t* temp = NULL;
    while (pointer) {
        temp = pointer->next;
        destroy_node(pointer);
        pointer = temp;
    }

    free(dlinkedlist);
    return 0;
}