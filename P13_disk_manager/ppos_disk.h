// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// tipo da solicitação
enum request_types { REQUEST_TYPE_WRITE = 0, REQUEST_TYPE_READ };

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.
typedef struct request_t {
    struct request_t *next, *prev; // ponteiros para usar em fila
    enum request_types type; // tipo da solicitação
    task_t *task; // task da solicitação
    int block; // índice do bloco da solicitação
    char *buf; // buffer de escrita/leitura
} request_t;

// estrutura que representa um disco no sistema operacional
typedef struct {
    task_t *task_queue; // fila de tarefas do disco
    request_t *req_queue; // fila de solicitações do disco
    request_t *req; // solicitação do disco
    semaphore_t semaphore; // semáforo
} disk_t;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

#endif