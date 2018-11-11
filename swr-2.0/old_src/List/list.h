/**
 * A dynamic list library.
 * This library uses a singlely linked list and void pointers to
 * be able to have a dynamic list of any type. This contains
 * the definitions for the library.
 *
 * @author  Joel McBeth
 * @version 1.0     02/10/05
 */

#ifndef list_h
#define list_h

#define FALSE   0
#define TRUE    !FALSE

typedef struct node NODE;
typedef struct list LIST;

/**
 * This is a node in the linked list which contains data
 * and the next node in the list.
 */
struct node
{
    void*   data;   /* This will point to the data that will be stored in this node.            */    
    NODE*   next;   /* This points to the next node in the list or NULL if it is the last node. */
};

/**
 * The keeps up with the head of the list and the length.
 */
struct list
{    
    int     length; /* The number of elements in the list.  */    
    NODE*   head;   /* The first node in the list.          */
};

NODE*   new_node        (void);
NODE*   new_node        (void* data);
void    free_node       (NODE* node);

LIST*   new_list        (void);
void    free_list       (LIST* list);

void    list_add        (LIST* list, void* data);
void    list_insert     (LIST* list, int index, void* data);

void    list_remove     (LIST* list, void* data);
void    list_remove_at  (LIST* list, int index);

void    list_clear      (LIST* list);

bool    list_contains   (LIST* list, void* data);

void*   list_item       (LIST* list, int index);

int     list_length     (LIST* list);

#endif