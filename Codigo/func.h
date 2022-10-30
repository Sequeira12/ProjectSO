//Bruno Sequeira (PL8) Diogo Rafael Tavares (PL7)
#ifndef MOBILENODE_C_FUNC_H
#define MOBILENODE_C_FUNC_H

#include <stdio.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#define PIPENAME "TASK_PIPE"






pthread_cond_t Ordena;
pthread_cond_t dispatcher_worker;
pthread_cond_t Monitor_Cond;
pthread_cond_t destroi_cond;

pthread_mutex_t destroi_mutex;
pthread_mutex_t Task ;
pthread_mutex_t scheduler_mutex;
pthread_mutex_t maintenance_mutex;
pthread_mutex_t monitor_mutex ;
pthread_mutex_t dispatcher_mutex;




typedef struct {
    int id_tarefa;
    int NInstrucoes;
    int tempoMax;
    int tempo;
}mobile;

// ############################### LISTA ####################################################### //
typedef struct no_filaS {
    mobile mobile;
    struct no_filaS *seg;
    
}no_filaS;

typedef struct lista {
    struct no_filaS * raiz;
    int tamanho;
}lista;


typedef struct Vcpu {
    char nome[50];
    int indice;
    int NSV;
    int tempo;
}vcpu;

typedef struct EdgeServer {
    int pid;
    char nome[50];			
    int cap1;				
    int cap2;              
    bool livres[2];
    int pipe[2];
    vcpu InfVcpu[2];
    int sig[2];
    float TemposVcpu[2];
    int manu;
    int done;   
    pthread_t my_thread[2];
    int Selecionado[2];
    int taskExecuted;
    int ServerManutencao;
    pthread_cond_t Manutencao;
    pthread_cond_t Vcpu[2];
    pthread_mutex_t Mvcpu[2];
    int TaskExec[2];
}edge;

typedef struct message {
   long mtype;
   int time;
} message;

typedef struct memory {
    char taskNOT[1000];
    int pidSystem;
    int pidMonitor;
    int pidMaintenance;
    int pidTask;
    int ExecutedTasks;
    int NotExecutedTask;
    int tempoMedio;
    int termina;
    int numeroSlots;
    int maxWait;
    int NedgeServers;
    pthread_mutex_t mutex;
    struct lista list;
    int Tasks;
    int Performance;
    edge Server[];
}memory;

int shmid;
int mqid;
sem_t * mutex_logFile;
sem_t * mutex_Shared;




pthread_mutex_t maintenance_mutex;
pthread_mutex_t monitor_mutex;
pthread_mutex_t server_mutex;
pthread_mutex_t scheduler_mutex;
pthread_mutex_t dispatcher_mutex;
memory *sharedDados;

void HandlerSIGURS2(int signum);
void HandlerSIGTERM(int signum);
void HandlerSIGINT(int signum);
void HandlerURS1(int signum);
void HandlerSIGTSTP(int signum);

bool colocar_lista(struct lista *pf, mobile t);
void inicializar(struct lista *pf);
void estatisticas();
int CalculaServersFree();
bool verificaVCPU();
void *VCPU(void *t);
void lerFicheiro(char *filename);
void SystemManager(char *filename);
void finalizar();
void taskManager();
void monitor();
void maintenance();
void *Scheduler(void *t);
void *Dispatcher(void *t);
void createServers(int i);
void logText(char mensagem[]);
void retirar(struct lista *p, mobile t);
void Junta(char palavra[100]);
#endif //MOBILENODE_C_FUNC_H
