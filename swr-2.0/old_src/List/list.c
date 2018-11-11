/**
 * A dynamic list library.
 * This library uses a singlely linked list and void pointers to
 * be able to have a dynamic list of any type. This contains the
 * implementations for the library.
 *
 * @author  Joel McBeth
 * @version 1.0     02/10/05
 */

/**
 * Allocates the memory and initilizes to default values.
 *
 * @version 1.0
 *
 * @returns A initialized and allocated node.
 */
NODE* new_node()
{
    return new_node(NULL);
}

/**
 * Allocates the memory and initilizes values for a node.
 *
 * @version 1.0
 *
 * @param   data    What to initialize the data to.
 *
 * @returns A initialized and allocated node.
 */
NODE* new_node(void* data)
{
    NODE* node;

    node = malloc(sizeof(node));

    node->data = data;
    node->next = NULL;

    return node;
}

/**
 * Frees a node from memory.
 * This does not free the next node or the data bececause
 * the next node might still be apart of memory and the
 * data might be used outside the scope of the list.
 *
 * @version 1.0
 *
 * @param   node    The node to free.
 */
void free_node(NODE* node)
{
    /* Can't free a NULL node. */
    if (node == NULL)
        return;

    free(node);

    node = NULL;
}

/**
 * Allocate memory and initialize the values of a list.
 *
 * @version 1.0     02/10/05
 *
 * @returns A pointers to a list that has be allocated and initialized.
 */
LIST* new_list(void)
{
    LIST* new_list;
    
    new_list = malloc(sizeof(LIST));

    new_list->length = 0;    
    new_list->head = NULL;

    return new_list;
}

/**
 * Go through and free all the memory that was allocated for the list.
 * This should be called when you are done using the list.
 * 
 * @version 1.0     02/10/05
 *
 * @param   list    The list to free.
 */
void free_list(LIST* list)
{
    NODE* ref_node;
    NODE* temp_node;

    /* Can't free a NULL list. */
    if (list == NULL)
        return;

    ref_node = list->head;

    /* Iterate through the list. */
    while (ref_node != NULL)
    {        
        /* Save the current node. */
        temp_node = ref_node;

        /* Go to the next node. */
        ref_node = ref_node->next;

        /* Now free the node we saved. */
        free_node(temp_node);        
    }

    /* Now free the list. */
    free(lst);
    lst = NULL;
}

/**
 * Adds an element to the front of the list.
 * If the list is NULL then the function returns.
 *
 * @version 1.0     2/10/05
 *
 * @param   list     The list to add the data to.
 * @param   data    The data to add to the list.
 */
void list_add(LIST* list, void* data)
{
    NODE* new_node;

    if (list == NULL)    
        return;

    new_node = malloc(sizeof(node));
    new_node->data = data;

    /**
     * We are adding this on to the beginning of the list.  So we set the current head as the next node.
     * It doesn't matter if there are no elements in the list, if there are none then
     * <code>list.head</code> should be equal to NULL and then you will be setting the next value to
     * NULL, and this is a null terminated list.
     */
    new_node->next = lst->head;

    /* Make the new node the head of the list. */
    list->head = new_node;

    list->length++;
}

/**
 * Inserts an element into the list at the specified index.
 * If the list is NULL or the index is out of bounds the function returns.
 *
 * @version 1.0     2/10/05
 *
 * @param   lst     The list to insert the element into.
 * @param   index   The position in the list to insert the element.
 * @param   data    The data to insert into the list.
 */
void list_insert(LIST* list, int index, void* data)
{
    NODE* new_node;
    NODE* prev_node;
    NODE* ref_node;
    int i;

    /* Can't do much with a NULL list. */
    if (list == NULL)
        return;

    /* Invalid index. */
    if ((index < 0) || (index >= list->length))
        return;

    new_node = malloc(sizeof(node));
    new_node->data = data;

    /* TODO: Finish me. */

    /* If someone put 0 as the index the just add it at the beginning. */
    if (index == 0)
    {
        new_node->next = lst->head;
        lst->head = new_node;
    }
    else
    {
        /* Otherwise go through the list untill the index is reached and then add it. */

        prev_node = lst->head;
        ref_node = prev_node->next;

        i = 1;

        do 
        {
            /* If we get to the index insert the node. */
            if (index == i)
            {
                prev_node->next = new_node;
                new_node->next = ref_node;

                break;
            }

            i++;

            prev_node = ref_node;
            ref_node = ref_node->next;

        } while (ref_node != NULL);
    }
}

void list_remove(list* lst, void* data)
{
    node* prev_node;
    node* ref_node;
    node* temp_node;

    if (lst == NULL)
        return;    

    ref_node = lst->head;

    /* Handle if the head contains the data to be removed. */
    if (ref_node->data == *data)
    {
        ref_node = ref_node->next;
    }
    else
    {
        prev = 
        do
        {
        } while (ref_node != NULL);
    }
}

void list_remove_at(list* lst, int index);

void list_clear(list* lst)
{
}

bool list_contains(list* lst, void* data);

void* list_item(list* lst, int index);

int list_length(list* lst);