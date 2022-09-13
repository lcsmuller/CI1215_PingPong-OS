// GRR20197160 Lucas Müller

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "ppos.h"
#include "queue.h"
#include "ppos_disk.h"
#include "disk.h"

extern task_t *g_task_curr; // tarefa atual
extern queue_t *g_queue; // fila de tarefa

static task_t g_task_disk; // tarefa do gerenciador de disco
static disk_t g_disk; // gerenciador de disco
static bool g_is_ready; // se o gerenciador de disco está pronto para escrita

static void
_disk_set_suspended(void)
{
    queue_remove(&g_queue, (queue_t *)&g_task_disk);
    g_task_disk.status = TASK_SUSPENDED;
}

static void
_disk_set_ready(void)
{
    queue_append(&g_queue, (queue_t *)&g_task_disk);
    g_task_disk.status = TASK_READY;
}

static void
_task_set_suspended(task_t *task)
{
    queue_remove(&g_queue, (queue_t *)task);
    queue_append((queue_t **)&g_disk.task_queue, (queue_t *)task);
    task->status = TASK_SUSPENDED;
}

static void
_task_set_ready(task_t *task)
{
    queue_remove((queue_t **)&g_disk.task_queue, (queue_t *)task);
    queue_append(&g_queue, (queue_t *)task);
    task->status = TASK_READY;
}

static void
_disk_manager(void *arg)
{
    (void)arg;
    while (1) {
        sem_down(&g_disk.semaphore);
        if (g_is_ready) {
            g_is_ready = false;

            _task_set_ready(g_disk.req->task);
            if (!queue_remove((queue_t **)&g_disk.req_queue,
                              (queue_t *)g_disk.req)) {
                free(g_disk.req);
                g_disk.req = NULL;
            }
        }
        const int status = disk_cmd(DISK_CMD_STATUS, 0, 0);
        if (status == 1 && g_disk.req_queue) {
            g_disk.req = g_disk.req_queue;
            const int cmd = g_disk.req->type == REQUEST_TYPE_READ
                                ? DISK_CMD_READ
                                : DISK_CMD_WRITE;
            disk_cmd(cmd, g_disk.req->block, g_disk.req->buf);
        }
        sem_up(&g_disk.semaphore);

        _disk_set_suspended();
        task_yield();
    }
}

static void
_disk_sighandler(int signum)
{
    (void)signum;
    if (g_is_ready = true, g_task_disk.status == TASK_SUSPENDED) {
        _disk_set_ready();
    }
}

int
disk_mgr_init(int *n_blocks, int *block_size)
{
    static struct sigaction action = { .sa_handler = _disk_sighandler };

    if (disk_cmd(DISK_CMD_INIT, 0, 0)) return -1;
    if ((*n_blocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0)) < 0) return -1;
    if ((*block_size = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0)) < 0) return -1;

    sem_create(&g_disk.semaphore, 1);
    task_create(&g_task_disk, &_disk_manager, NULL);

    sigemptyset(&action.sa_mask);
    if (sigaction(SIGUSR1, &action, 0) < 0) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    g_task_disk.status = TASK_SUSPENDED;

    return 0;
}

static request_t *
_request_init(enum request_types type, int block, char buf[])
{
    request_t *req;

    if (!(req = malloc(sizeof *req))) {
        perror("malloc()");
        return NULL;
    }
    *req = (request_t){
        .type = type,
        .task = g_task_curr,
        .block = block,
        .buf = buf,
    };

    return req;
}

int
disk_block_read(int block, void *buf)
{
    request_t *req;

    sem_down(&g_disk.semaphore);
    if (!(req = _request_init(REQUEST_TYPE_READ, block, buf))) {
        sem_up(&g_disk.semaphore);
        return -1;
    }
    queue_append((queue_t **)&g_disk.req_queue, (queue_t *)req);
    if (g_task_disk.status == TASK_SUSPENDED) _disk_set_ready();
    sem_up(&g_disk.semaphore);

    _task_set_suspended(g_task_curr);
    task_yield();

    return 0;
}

int
disk_block_write(int block, void *buf)
{
    request_t *req;

    sem_down(&g_disk.semaphore);
    if (!(req = _request_init(REQUEST_TYPE_WRITE, block, buf))) {
        sem_up(&g_disk.semaphore);
        return -1;
    }
    queue_append((queue_t **)&g_disk.req_queue, (queue_t *)req);
    if (g_task_disk.status == TASK_SUSPENDED) _disk_set_ready();
    sem_up(&g_disk.semaphore);

    _task_set_suspended(g_task_curr);
    task_yield();

    return 0;
}
