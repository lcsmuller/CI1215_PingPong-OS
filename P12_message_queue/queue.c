// GRR20197160 Lucas Müller

#include <stdio.h>
#include "queue.h"

//------------------------------------------------------------------------------
// Retorna código de erro se condição 'expect' não for verdadeira
//
#define EXPECT(expect, code, reason)                                          \
    do {                                                                      \
        if (!(expect)) {                                                      \
            fprintf(stderr, "Expected: " #expect ": " reason "\n");           \
            return code;                                                      \
        }                                                                     \
    } while (0)

int
queue_size(queue_t *q)
{
    if (!q) return 0;
    int size = 1;
    for (queue_t const *end = q, *it = q->next; it != end; it = it->next)
        ++size;
    return size;
}

void
queue_print(char *name, queue_t *q, void print_elem(void *))
{
    printf("%s [", name);
    if (q) {
        queue_t const *end = q;
        while (print_elem(q), (q = q->next) != end)
            putchar(' ');
    }
    puts("]");
}

int
queue_append(queue_t **q, queue_t *elem)
{
    EXPECT(q != NULL, -1, "A fila deve existir!");
    EXPECT(elem != NULL, -2, "O elemento deve existir!");
    EXPECT(!elem->next, -3, "O elemento já pertence a uma fila!");
    if (*q == NULL) {
        *q = elem;
        (*q)->next = (*q)->prev = *q;
    }
    else {
        elem->next = *q;
        elem->prev = (*q)->prev;
        (*q)->prev = elem->prev->next = elem;
    }
    return 0;
}

int
queue_remove(queue_t **q, queue_t *elem)
{
    EXPECT(q != NULL, -1, "A fila deve existir!");
    EXPECT(elem != NULL, -2, "O elemento deve existir!");
    EXPECT(*q != NULL, -3, "A fila não deve estar vazia!");
    if (*q == elem) {
        if (*q == (*q)->next) // fila com um único elemento
            *q = NULL;
        else { // elemento no topo da fila
            *q = elem->prev->next = elem->next;
            elem->next->prev = elem->prev;
        }
        elem->next = elem->prev = NULL;
        return 0;
    }
    for (queue_t const *end = *q, *it = (*q)->next; it != end; it = it->next) {
        if (it == elem) {
            elem->prev->next = elem->next;
            elem->next->prev = elem->prev;
            elem->next = elem->prev = NULL;
            return 0;
        }
    }
    fputs("Elemento não pertence à fila\n", stderr);
    return -4;
}
