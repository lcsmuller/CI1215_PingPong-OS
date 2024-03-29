// GRR20197160 Lucas Müller

#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"
#include "queue.h"

#ifdef DEBUG
#define LOG_INFO(...)                                                         \
    do {                                                                      \
        fprintf(stderr, "%s(): ", __func__);                                  \
        fprintf(stderr, __VA_ARGS__);                                         \
    } while (0)
#else
#define LOG_INFO(...)
#endif

#define STACK_SIZE 64 * 1024 // tamanho da stack de cada tarefa

#define PRIO_ALPHA -1 // fator de envelhecimento de prioridade
#define PRIO_MAX   -20 // priodade máxima permitida
#define PRIO_MIN   20 // prioridade minima permitida

enum task_status {
    TASK_READY = 0, // setado no task_create()
    TASK_FINISHED, // setado no task_exit()
    TASK_RUNNING, // setado no task_switch()
    TASK_SUSPENDED // setado no task_switch()
};

static task_t g_task_main, g_task_dispatcher; // tarefa main e dispatcher
static task_t *g_task_curr, *g_task_prev; // ponteiros tarefa atual e anterior
static queue_t *g_queue; // fila de tarefas
static int g_num_tasks; // quantidade de tarefas criadas

// cria a tarefa sem adicionar à fila
static int
_task_create(task_t *task, void (*start_routine)(void *), void *arg)
{
    static int id; // ID da tarefa
    char *st; // ponteiro para stack

    *task = (task_t){ .id = ++id };
    if (getcontext(&task->context) == -1) {
        perror("Erro: ");
        return -1;
    }
    if (!(st = malloc(STACK_SIZE))) {
        fputs("Erro: Não foi possível alocar memória\n", stderr);
        return -2;
    }
    task->context.uc_stack = (stack_t){ .ss_sp = st, .ss_size = STACK_SIZE };

    makecontext(&task->context, (void (*)())start_routine, 1, arg);

    return id;
}

// limpa recursos e remove tarefa da fila
static void
_task_delete(task_t *task)
{
    free(task->context.uc_stack.ss_sp);
    queue_remove(&g_queue, (queue_t *)g_task_prev);
    --g_num_tasks;

    LOG_INFO("num tarefas: %d ; tarefas na fila: %d\n", g_num_tasks,
             queue_size(g_queue));
}

static task_t *
_scheduler(void)
{
    task_t *high = (task_t *)g_queue;

    for (task_t *it = high->next; it != (task_t *)g_queue; it = it->next) {
        if (high->prio_incr <= it->prio_incr)
            it->prio_incr += PRIO_ALPHA;
        else {
            high->prio_incr += PRIO_ALPHA;
            high = it;
        }
    }
    high->prio_incr = high->prio; // reset para próximos usos
    return high;
}

static void
_dispatcher(void *arg)
{
    (void)arg;

    while (g_num_tasks > 0) { // enquanto houver tarefas do usuário
        task_t *task = _scheduler();
        // se a task trocada tiver sido finalizada, então é deletada
        if (task_switch(task), g_task_prev->status == TASK_FINISHED)
            _task_delete(g_task_prev);
    }
    // encerra a tarefa dispatcher
    task_exit(0);
}

void
ppos_init(void)
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);
    g_task_curr = &g_task_main;
    // inicia tarefa dispatcher
    _task_create(&g_task_dispatcher, _dispatcher, NULL);
}

int
task_create(task_t *task, void (*start_routine)(void *), void *arg)
{
    int id = _task_create(task, start_routine, arg);
    queue_append(&g_queue, (queue_t *)task);
    ++g_num_tasks;

    LOG_INFO("criou tarefa %d (tam fila: %d)\n", id, queue_size(g_queue));

    return id;
}

int
task_switch(task_t *task)
{
    g_task_prev = g_task_curr;
    g_task_curr = task;

    if (g_task_prev->status != TASK_FINISHED)
        g_task_prev->status = TASK_SUSPENDED;
    task->status = TASK_RUNNING;

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
    task_t *task = (g_task_curr != &g_task_dispatcher) ? &g_task_dispatcher
                                                       : &g_task_main;

    LOG_INFO("tarefa %d sendo encerrada\n", g_task_curr->id);
    g_task_curr->status = TASK_FINISHED;
    task_switch(task);
}

int
task_id(void)
{
    return g_task_curr->id;
}

void
task_yield(void)
{
    /* move tarefa para o fim da fila (se não for tarefa main) */
    if (g_task_curr != &g_task_main) {
        queue_remove(&g_queue, (queue_t *)g_task_curr);
        queue_append(&g_queue, (queue_t *)g_task_curr);
    }
    LOG_INFO("tarefa %d sendo trocada\n", g_task_curr->id);
    task_switch(&g_task_dispatcher);
}

void
task_setprio(task_t *task, int prio)
{
    if (!task) task = g_task_curr;

    if (prio < PRIO_MAX)
        prio = PRIO_MAX;
    else if (prio > PRIO_MIN)
        prio = PRIO_MIN;
    task->prio = task->prio_incr = prio;
}

int
task_getprio(task_t *task)
{
    return task ? task->prio : g_task_curr->prio;
}
