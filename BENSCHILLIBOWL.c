#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = {
    "BensChilli",
    "BensHalfSmoke",
    "BensHotDog",
    "BensChilliCheeseFries",
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = (BENSCHILLIBOWL *) malloc(sizeof(BENSCHILLIBOWL));
    if (bcb == NULL) {
        printf("*** malloc error: could not open restaurant ***\n");
        exit(1);
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    srand(time(NULL));

    printf("Restaurant is open!\n");
    return bcb;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    assert(bcb->orders_handled == bcb->expected_num_orders);
    assert(bcb->current_size == 0);
    assert(bcb->orders == NULL);

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    printf("Orders handled: %d of %d expected\n", bcb->orders_handled, bcb->expected_num_orders);

    free(bcb);

    printf("Restaurant is closed!\n");
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    /* Wait until there is room in the queue */
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    order->order_number = bcb->next_order_number;
    bcb->next_order_number++;

    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    int order_number = order->order_number;

    /* Wake one cook: there is now an order to take */
    pthread_cond_signal(&bcb->can_get_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    /* Wait while the queue is empty but more orders are still coming */
    while (IsEmpty(bcb) && bcb->orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    /* Queue is empty and every expected order has been taken: no work left */
    if (IsEmpty(bcb)) {
        pthread_cond_broadcast(&bcb->can_get_orders);
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    /* Remove the order at the front of the queue */
    Order *order = bcb->orders;
    bcb->orders = order->next;
    order->next = NULL;

    bcb->current_size--;
    bcb->orders_handled++;

    /* That was the last order: wake every waiting cook so they can go home */
    if (bcb->orders_handled == bcb->expected_num_orders) {
        pthread_cond_broadcast(&bcb->can_get_orders);
    }

    /* Wake one customer: there is now room in the queue */
    pthread_cond_signal(&bcb->can_add_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) {
  return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
  return bcb->current_size >= bcb->max_size;
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
    order->next = NULL;

    if (*orders == NULL) {
        *orders = order;
        return;
    }

    Order *current = *orders;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = order;
}