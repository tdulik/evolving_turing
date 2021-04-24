/*
 * Copyright 2010 Tomas Dulik <dulik@fai.utb.cz>
 * Copyright 2010 Volkan Yazıcı <volkan.yazici@gmail.com>
 * Copyright 2006-2010 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */


/**
 * @file  pqueue.h
 * @brief Priority Queue implemented by binary heap - the function declarations
 *
 * @{
 */


#ifndef PQUEUE_H
#define PQUEUE_H
#include "evolve_turing.h"

/************************************************************************/
/** @struct pqueue_t
 * the priority queue handle
 */
typedef struct pqueue_t {
    tIndividual ** d;	/**< the queue is implemented as this array */
    size_t size; 		/**< the size of the array */
    size_t avail;		/**< number of available items */
} pqueue_t;


/**
 * initialize the queue
 * @param n the initial estimate of the number of queue items for which memory
 *          should be preallocated
 * @Return the handle or NULL for insufficent memory
 */
pqueue_t * pqueue_init(size_t n);


/**
 * free all memory used by the queue
 * @param q the queue
 */
void pqueue_free(pqueue_t *q);


/**
 * return the size of the queue.
 * @param q the queue
 */
size_t pqueue_size(pqueue_t *q);

/**
 * Resets the queue to initial (empty) state.
 * @param q the queue
 */

inline void pqueue_reset(pqueue_t *q);

/**
 * insert an item into the queue.
 * @param q the queue
 * @param d the item
 * @return 0 on success
 */
int pqueue_insert(pqueue_t *q, tIndividual *d);


/**
 * pop the highest-ranking item from the queue.
 * @param p the queue
 * @param d where to copy the entry to
 * @return NULL on error, otherwise the entry
 */
tIndividual *pqueue_pop(pqueue_t *q);

/**
 * move an existing entry to a different priority
 * @param q the queue
 * @param old the old priority
 * @param d the entry
 */
void pqueue_change_priority(pqueue_t *q, double new_pri, size_t i);

int pqueue_priority_changed(pqueue_t *q, double old_pri, size_t i);

/**
 * remove an item from the queue.
 * @param p the queue
 * @param d the entry
 * @return 0 on success
 */
int pqueue_remove(pqueue_t *q, size_t i);


/**
 * access highest-ranking item without removing it.
 * @param q the queue
 * @param d the entry
 * @return NULL on error, otherwise the entry
 */
tIndividual *pqueue_peek(pqueue_t *q);

/**
 * access i-th item (without removing it).
 * @param q the queue
 * @param i the item number. Warning: the first item has i=1, the last item has i=q->size
 * @return NULL on error, otherwise the entry
 */
inline tIndividual * pqueue_get(pqueue_t *q, size_t i);

#endif /* PQUEUE_H */
/** @} */
