#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"

#ifdef DEBUG
#define LOG_INFO(...)                                                         \
    do {                                                                      \
        fprintf(stderr, "%s(): ", __func__);                                  \
        fprintf(stderr, __VA_ARGS__);                                         \
    } while (0)
#else
#define LOG_INFO(...)
#endif

static task_t g_task_main, *g_task_curr, *g_task_prev;

void
ppos_init(void)
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);
    g_task_curr = &g_task_main;
}

#define STACK_SIZE 64 * 1024
int
task_create(task_t *task, void (*start_routine)(void *), void *arg)
{
    static int id; // ID da tarefa
    char *st; // ponteiro para stack

    *task = (task_t){ .id = ++id, .func = start_routine, .arg = arg };
    if (getcontext(&task->context) == -1) {
        perror("Erro: ");
        return -1;
    }
    if (!(st = malloc(STACK_SIZE))) {
        fputs("Erro: Não foi possível alocar memória\n", stderr);
        return -2;
    }
    task->context.uc_stack = (stack_t){ .ss_sp = st, .ss_size = STACK_SIZE };

    makecontext(&task->context, (void (*)())task->func, 1, task->arg);
    LOG_INFO("criou tarefa %d\n", id);

    return id;
}
#undef STACK_SIZE

int
task_switch(task_t *task)
{
    g_task_prev = g_task_curr;
    g_task_curr = task;

    LOG_INFO("trocando contexto %d - %d\n", g_task_prev->id, task->id);
    if (swapcontext(&g_task_prev->context, &task->context) == -1) {
        perror("Erro: ");
        return -1;
    }

    return 0;
}

void
task_exit(int exit_code)
{
    (void)exit_code;
    LOG_INFO("tarefa %d sendo encerrada\n", g_task_curr->id);
    task_switch(&g_task_main);
}

int
task_id(void)
{
    return g_task_curr->id;
}
