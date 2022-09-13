// GRR20197160 Lucas Müller

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

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

task_t *g_task_curr; // ponteiro para tarefa atual
queue_t *g_queue; // fila de tarefas

static task_t g_task_main, g_task_dispatcher; // tarefa main e dispatcher
static task_t *g_task_prev; // ponteiros para tarefa anterior
static queue_t *g_queue_asleep; // fila de tarefas adormecidas
static int g_num_tasks; // quantidade de tarefas criadas

#define QUANTUM_TICKS         20 // quantum inicial da tarefa
#define QUANTUM_PREEMPTION_MS 1U // preempção do quantum em milisegundos

static int g_quantum = QUANTUM_TICKS; // decrementa a cada tick
static unsigned g_ticks; // contabilização de ticks

// cria a tarefa sem adicionar à fila
static int
_task_create(task_t *task, void (*start_routine)(void *), void *arg)
{
    static int id; // ID da tarefa

    *task = (task_t){ .id = id++ };
    if (getcontext(&task->context) == -1) {
        perror("getcontext(): ");
        return -1;
    }
    if (start_routine) {
        task->context.uc_stack =
            (stack_t){ .ss_sp = malloc(STACK_SIZE), .ss_size = STACK_SIZE };
        if (!task->context.uc_stack.ss_sp) {
            fputs("malloc(): Não foi possível alocar memória\n", stderr);
            return -2;
        }
        makecontext(&task->context, (void (*)())start_routine, 1, arg);
    }
    task->status = TASK_READY;
    task->exec_time = systime();

    return id;
}

// limpa recursos e remove tarefa da fila
static void
_task_delete(task_t *task)
{
    if (task->context.uc_stack.ss_sp) free(task->context.uc_stack.ss_sp);
    queue_remove(&g_queue, (queue_t *)task);
    --g_num_tasks;

    LOG_INFO("num tarefas: %d ; tarefas na fila: %d\n", g_num_tasks,
             queue_size(g_queue));
}

static task_t *
_scheduler(void)
{
    task_t *high = (task_t *)g_queue;

    if (!high) return NULL;

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
    task_t *task;

    while (g_num_tasks > 0) { // enquanto houver tarefas do usuário
        const unsigned k_systime = systime();
        // checa se há tarefas a serem acordadas pendentes
        task = (task_t *)g_queue_asleep;
        for (int i = 0, n = queue_size(g_queue_asleep); i < n; ++i) {
            task = task->next;
            if (task->prev->wake_tstamp <= k_systime)
                task_resume(task->prev, (task_t **)&g_queue_asleep);
        }
        if ((task = _scheduler()) == NULL) continue;
        // troca tarefa, e se a task trocada tiver sido finalizada,
        //      então é deletada
        if (task_switch(task), g_task_prev->status == TASK_FINISHED)
            _task_delete(g_task_prev);
    }
    // encerra a tarefa dispatcher
    task_exit(0);
}

static void
_task_tick(int signum)
{
    (void)signum;
    ++g_ticks;
    if (g_task_curr != &g_task_dispatcher && 0 == --g_quantum) {
        task_yield();
    }
}

void
ppos_init(void)
{
    static struct sigaction action = { .sa_handler = _task_tick };
    static struct itimerval timer = { .it_value.tv_usec =
                                          QUANTUM_PREEMPTION_MS * 1000,
                                      .it_interval.tv_usec =
                                          QUANTUM_PREEMPTION_MS * 1000 };

    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);
    g_task_curr = &g_task_main;
    // inicia tarefa main
    task_create(&g_task_main, NULL, NULL);
    // inicia tarefa dispatcher
    _task_create(&g_task_dispatcher, _dispatcher, NULL);

    // inicia temporizador
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("sigaction(): ");
        exit(EXIT_FAILURE);
    }
    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("setitimer(): ");
        exit(EXIT_FAILURE);
    }

    task_yield();
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
    static unsigned ticks_prev; // armazena ticks do momento de chamada

    if (!task) return -1;

    g_task_prev = g_task_curr;
    g_task_curr = task;

    if (g_task_prev->status != TASK_FINISHED) {
        g_task_prev->proc_time += systime() - ticks_prev;
        g_task_prev->status = TASK_SUSPENDED;
    }
    task->status = TASK_RUNNING;

    ticks_prev = systime();

    g_quantum = QUANTUM_TICKS; // reseta quantum corrente

    ++task->activations; // ativações da tarefa

    LOG_INFO("trocando contexto %d - %d\n", g_task_prev->id, task->id);
    if (swapcontext(&g_task_prev->context, &task->context) == -1) {
        perror("swapcontext(): ");
        return -1;
    }
    return 0;
}

void
task_exit(int exit_code)
{
    task_t *task = (g_task_curr != &g_task_dispatcher) ? &g_task_dispatcher
                                                       : &g_task_main;

    g_task_curr->exec_time = systime() - g_task_curr->exec_time;

    printf("Task %d exit (%d): execution time %u ms, processor time %u ms, %u "
           "activations\n",
           g_task_curr->id, exit_code, g_task_curr->exec_time,
           g_task_curr->proc_time, g_task_curr->activations);

    // resume todas tarefas da fila de espera
    for (task_t *head; (head = g_task_curr->wait_queue) != NULL;)
        task_resume(head, &g_task_curr->wait_queue);

    LOG_INFO("tarefa %d sendo encerrada\n", g_task_curr->id);
    g_task_curr->exit_code = exit_code;
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

static void
_task_suspend_no_yield(task_t **queue)
{
    queue_remove(&g_queue, (queue_t *)g_task_curr);
    g_task_curr->status = TASK_SUSPENDED;
    queue_append((queue_t **)queue, (queue_t *)g_task_curr);
}

void
task_suspend(task_t **queue)
{
    _task_suspend_no_yield(queue);
    task_yield();
}

void
task_resume(task_t *task, task_t **queue)
{
    if (!queue) return;
    queue_remove((queue_t **)queue, (queue_t *)task);
    task->status = TASK_READY;
    queue_append(&g_queue, (queue_t *)task);
}

int
task_join(task_t *task)
{
    if (!task || task->status == TASK_FINISHED) return -1;
    task_suspend(&task->wait_queue);
    return task->exit_code;
}

void
task_sleep(int t)
{
    g_task_curr->wake_tstamp = systime() + (unsigned)((t < 0) ? 0 : t);
    task_suspend((task_t **)&g_queue_asleep);
}

unsigned int
systime(void)
{
    return g_ticks * QUANTUM_PREEMPTION_MS;
}

int
sem_create(semaphore_t *s, int value)
{
    if (!s) return -1;
    *s = (semaphore_t){ .value = value };
    return 0;
}

static void
_sem_lock_cs_enter(int *lock)
{
    while (__sync_fetch_and_or(lock, 1))
        continue;
}

static void
_sem_lock_cs_leave(int *lock)
{
    *lock = 0;
}

int
sem_down(semaphore_t *s)
{
    if (!s || s->is_destroyed) return -1;

    _sem_lock_cs_enter(&s->lock);
    if (--s->value >= 0)
        _sem_lock_cs_leave(&s->lock);
    else {
        _task_suspend_no_yield(&s->suspended_tasks);
        _sem_lock_cs_leave(&s->lock);
        task_yield();
    }
    return 0;
}

int
sem_up(semaphore_t *s)
{
    if (!s || s->is_destroyed) return -1;

    _sem_lock_cs_enter(&s->lock);
    if (++s->value >= 0 && s->suspended_tasks)
        task_resume(s->suspended_tasks, &s->suspended_tasks);
    _sem_lock_cs_leave(&s->lock);

    return 0;
}

int
sem_destroy(semaphore_t *s)
{
    if (!s || s->is_destroyed) return -1;

    while (s->suspended_tasks)
        task_resume(s->suspended_tasks, &s->suspended_tasks);
    s->is_destroyed = 1;

    return 0;
}

int
mqueue_create(mqueue_t *queue, int max, int size)
{
    if (!queue) return -1;

    *queue = (mqueue_t){
        .messages = {
            .buf = calloc(max, size),
            .cap = max,
            .size = size,
        },
    };
    if (sem_create(&queue->semaphores.send, max) < 0) return -1;
    if (sem_create(&queue->semaphores.recv, 0) < 0) return -1;
    if (sem_create(&queue->semaphores.write, 1) < 0) return -1;

    return 0;
}

int
mqueue_send(mqueue_t *queue, void *msg)
{
    if (!queue || !msg || queue->messages.is_destroyed) return -1;

    sem_down(&queue->semaphores.send);
    sem_down(&queue->semaphores.write);
    void *ptr_last =
        queue->messages.buf + (queue->messages.end++ * queue->messages.size);
    memcpy(ptr_last, msg, queue->messages.size);
    // printf("Colocou mensagem na posição: %d\n", queue->messages.end);
    queue->messages.end %= queue->messages.cap;
    sem_up(&queue->semaphores.write);
    sem_up(&queue->semaphores.recv);

    return 0;
}

int
mqueue_recv(mqueue_t *queue, void *msg)
{
    if (!queue || queue->messages.is_destroyed) return -1;

    sem_down(&queue->semaphores.recv);
    sem_down(&queue->semaphores.write);
    const void *ptr_start =
        queue->messages.buf + (queue->messages.start++ * queue->messages.size);
    memcpy(msg, ptr_start, queue->messages.size);
    // printf("Leu mensagem da posição: %d\n", queue->messages.start);
    queue->messages.start %= queue->messages.cap;
    sem_up(&queue->semaphores.write);
    sem_up(&queue->semaphores.send);

    return 0;
}

int
mqueue_destroy(mqueue_t *queue)
{
    if (!queue) return -1;

    if (sem_destroy(&queue->semaphores.send) < 0) return -1;
    if (sem_destroy(&queue->semaphores.recv) < 0) return -1;
    if (sem_destroy(&queue->semaphores.write) < 0) return -1;
    queue->messages.is_destroyed = 1;
    free(queue->messages.buf);

    return 0;
}

int
mqueue_msgs(mqueue_t *queue)
{
    return queue ? (queue->messages.end - queue->messages.start)
                       / queue->messages.size
                 : -1;
}