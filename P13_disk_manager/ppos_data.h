// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h> // biblioteca POSIX de trocas de contexto

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t {
    struct task_t *prev, *next; // ponteiros para usar em filas
    int id; // identificador da tarefa
    ucontext_t context; // contexto armazenado da tarefa
    enum {
        TASK_READY = 1, // setado no task_create()
        TASK_FINISHED, // setado no task_exit()
        TASK_RUNNING, // setado no task_switch()
        TASK_SUSPENDED, // setado no task_suspend()
    } status; // pronta, rodando, suspensa, ...
    short preemptable; // pode ser preemptada?
    /* ... */
    int prio; // prioridade da tarefa (entre -20 e +20)
    int prio_incr; // incrementador de prioridade
    unsigned exec_time, proc_time; // tempo de execuçao e processamento
    unsigned activations; // ativações da tarefa
    int exit_code; // valor de saída da tarefa para o task_exit()
    struct task_t *wait_queue; // fila de espera da task
    unsigned wake_tstamp; // timestamp em que a task deve acordar (em ms)
} task_t;

// estrutura que define um semáforo
typedef struct {
    int value; // qtd de tarefas que podem "passar
    task_t *suspended_tasks; // tarefas suspensas
    int lock; // registra estado do lock
    _Bool is_destroyed; // 1 se semaphoro foi destruido, 0 caso contrário
} semaphore_t;

// estrutura que define um mutex
typedef struct {
    // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct {
    // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct {
    struct { // estrutura da fila de mensagem
        void *buf; // buffer de mensagens
        int cap; // quantidade total de msgs
        int size; // tamanho de cada mensagem
        int start, end; // começo e fim do buffer
        _Bool is_destroyed; // 1 se phila foi destruido, 0 caso contrário
    } messages;
    struct { // semáforos
        semaphore_t send; // semáforo de envio
        semaphore_t recv; // semáforo de recebimento
        semaphore_t write; // semáforo de escrita
    } semaphores;
} mqueue_t;

#endif
