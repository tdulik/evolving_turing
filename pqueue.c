/*
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pqueue.h"


#define left(i)   ((i) << 1)
#define right(i)  (((i) << 1) + 1)
#define parent(i) ((i) >> 1)
#define cmppri(a,b) (a->fitness < b->fitness)

pqueue_t * pqueue_init(size_t n)
{
    pqueue_t *q;

    if (!(q = malloc(sizeof(pqueue_t))))
        return NULL;

    /* Need to allocate n+1 elements since element 0 isn't used. */
    if (!(q->d = malloc((n + 1) * sizeof(tIndividual *)))) {
        free(q);
        return NULL;
    }

    q->size = 1;
    q->avail = (n+1);  /* see comment above about n+1 */
    return q;
}


void pqueue_free(pqueue_t *q)
{
    free(q->d);
    free(q);
}


size_t pqueue_size(pqueue_t *q)
{
    /* queue element 0 exists but doesn't count since it isn't used. */
    return (q->size - 1);
}

inline void pqueue_reset(pqueue_t *q) {
	q->size = 1;
}
static int bubble_up(pqueue_t *q, size_t i) {
    size_t parent_node;
    tIndividual * moving_node = q->d[i];

    for (parent_node = parent(i); 
		 i>1 && cmppri(q->d[parent_node],  moving_node);
         i = parent_node, parent_node = parent(i)) 
	{
        q->d[i] = q->d[parent_node];
    }

    q->d[i] = moving_node;
    return i;
}


static size_t maxchild(pqueue_t *q, size_t i) {
    size_t child_node = left(i);

    if (child_node >= q->size)
        return 0;

    if (child_node+1 < q->size && cmppri(q->d[child_node], q->d[child_node+1]))
        child_node++; /* use right child instead of left */

    return child_node;
}


static int
percolate_down(pqueue_t *q, size_t i)
{
    size_t child_node;
    tIndividual *moving_node = q->d[i];

	while ((child_node = maxchild(q, i)) && cmppri(moving_node, q->d[child_node]))
    {
        q->d[i] = q->d[child_node];
        i = child_node;
    }

    q->d[i] = moving_node;
    return i;
}


int
pqueue_insert(pqueue_t *q, tIndividual *d)
{
    size_t i;

    if (!q) return 1;

    /* allocate more memory if necessary */
    if (q->size >= q->avail) {
		fprintf(stderr, "Unexpected need for queue growth!\n");
		exit(-1);
	}

    /* insert item */
    i = q->size++;
    q->d[i] = d;
    bubble_up(q, i);

    return 0;
}


void pqueue_change_priority(pqueue_t *q, double new_pri, size_t i) {
    double old_pri = q->d[i]->fitness;

    q->d[i]->fitness=new_pri;
    if (old_pri<new_pri)
        bubble_up(q, i);
    else
        percolate_down(q, i);
}

int pqueue_priority_changed(pqueue_t *q, double old_pri, size_t i) {
    double new_pri = q->d[i]->fitness;
    int position=i;
    if (i >= q->size) {
		fprintf(stderr, "Unexpected need for queue growth!\n");
		exit(-1);
    }

    if (old_pri<new_pri)
        position=bubble_up(q, i);
    else
        position=percolate_down(q, i);
    return position;
}

int pqueue_remove(pqueue_t *q, size_t i) {
    double old_pri = q->d[i]->fitness;
    q->d[i] = q->d[--q->size];
    if (old_pri< q->d[i]->fitness)
        bubble_up(q, i);
    else
        percolate_down(q, i);

    return 0;
}


tIndividual * pqueue_pop(pqueue_t *q) {
    tIndividual *head;

    if (!q || q->size == 1)
        return NULL;

    head = q->d[1];
    q->d[1] = q->d[--q->size];
    percolate_down(q, 1);

    return head;
}


tIndividual * pqueue_peek(pqueue_t *q) {
    tIndividual *d;
    if (!q || q->size == 1)
        return NULL;
    d = q->d[1];
    return d;
}


void pqueue_dump(pqueue_t *q, FILE *out) {
    int i;

    fprintf(stdout,"posn\tleft\tright\tparent\tmaxchild\t...\n");
    for (i = 1; i < q->size ;i++) {
        fprintf(stdout,
                "%d\t%d\t%d\t%d\t%ul\t",
                i,
                left(i), right(i), parent(i),
                maxchild(q, i));
    }
}



void pqueue_print(pqueue_t *q, FILE *out)
{
    pqueue_t *dup;
	tIndividual *e;

    dup = pqueue_init(q->size);
    dup->size = q->size;
    dup->avail = q->avail;

    memcpy(dup->d, q->d, (q->size * sizeof(tIndividual *)));

    while ((e = pqueue_pop(dup)))
		fprintf(out, "Fitness=%lf\n", e->fitness);

    pqueue_free(dup);
}


static int subtree_is_valid(pqueue_t *q, int pos) {
    if (left(pos) < q->size) {
        /* has a left child */
        if (cmppri(q->d[pos], q->d[left(pos)]))
            return 0;
        if (!subtree_is_valid(q, left(pos)))
            return 0;
    }
    if (right(pos) < q->size) {
        /* has a right child */
        if (cmppri(q->d[pos], q->d[right(pos)]))
            return 0;
        if (!subtree_is_valid(q, right(pos)))
            return 0;
    }
    return 1;
}


int pqueue_is_valid(pqueue_t *q) {
    return subtree_is_valid(q, 1);
}

inline tIndividual * pqueue_get(pqueue_t *q, size_t i) {
	return q->d[i];
}
