#include "func.h"
//Bruno Sequeira (PL8) Diogo Rafael Tavares (PL7)


int THREAD = 0;
//##################################    HANDLERS ################################################################
void HandlerSIGURS2(int signum){
    pthread_cond_signal(&destroi_cond);
   
}
void HandlerSIGTERM(int signum){
    pthread_cond_signal(&Monitor_Cond);
}

void HandlerSIGINT(int signum){
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    sem_wait(mutex_logFile);
    logText("SIGNAL SIGINT RECEIVED");
    logText("SIMULATOR WAITING FOR LAST TASKS TO FINISH");
    sem_post(mutex_logFile);
    sem_wait(mutex_Shared);
    sharedDados->termina = 1;
    sem_post(mutex_Shared);

    kill(sharedDados->pidMonitor,SIGKILL);
    kill(sharedDados->pidMaintenance,SIGKILL);
    while(1){
        pthread_mutex_lock(&destroi_mutex);
        while(CalculaServersFree() != sharedDados->NedgeServers){
            pthread_cond_wait(&destroi_cond,&destroi_mutex);
        }
        for(int i = 0; i < sharedDados->NedgeServers; i++){
            kill(sharedDados->Server[i].pid,SIGKILL);
        }
        break;
        pthread_mutex_unlock(&destroi_mutex);
    }
    kill(sharedDados->pidTask,SIGKILL);

    sem_wait(mutex_logFile);
    logText(sharedDados->taskNOT);
    sem_post(mutex_logFile);
    estatisticas();

    finalizar();
}

void HandlerURS1(int signum){
    pthread_cond_signal(&dispatcher_worker);
    pthread_cond_signal(&Monitor_Cond);
}

void HandlerSIGTSTP(int signum){
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);

    sem_wait(mutex_logFile);
    logText("SIGNAL TSTP RECEIVED");
    sem_post(mutex_logFile);
    estatisticas();
    signal(SIGINT,HandlerSIGINT);
    signal(SIGTSTP,HandlerSIGTSTP);
}
//##########################################################################################################################################

// ##################################################### FUNÇÕES LISTA ################################################//
//criar lista
void inicializar(struct lista *pf){
    pf->raiz = NULL;
    pf->tamanho = 0;
}

//retirar mensagem da lista
void retirar(struct lista *p, mobile t){
    struct no_filaS *aux = p->raiz, *anterior;
    if(aux != NULL && aux->mobile.id_tarefa == t.id_tarefa){
        p->raiz = aux->seg;
        free(aux);
        p->tamanho--;
        return;
    }
    while(aux != NULL && (aux->mobile.id_tarefa != t.id_tarefa)){
        anterior= aux;
        aux = aux->seg;
    }
    if(aux == NULL){
        return;
    }
    anterior->seg = aux->seg;
    p->tamanho--;
    free(aux);
}


void Junta(char palavra[100]){
    strcpy(palavra,"Tarefas não executadas:");
    if(sharedDados->list.tamanho == 0){
        char buffer[50] = "Todas as tarefas foram executadas";
        strcat(palavra,buffer);
        return;
    }
    struct no_filaS *auxi;
    for(auxi = sharedDados->list.raiz; auxi != NULL; auxi = auxi->seg){
        char buffer[10];
        sprintf(buffer,"%d ",auxi->mobile.id_tarefa);
        strcat(palavra,buffer);
    }
}

//colocar mensagem na lista
bool colocar_lista(struct lista *pf, mobile teste) {
    struct no_filaS *aux, *anterior, *prox;
    aux = (struct no_filaS*)malloc(sizeof(struct no_filaS));
    if(aux == NULL){
        return false;
    }
    //colocar mensagem na fila
    aux->mobile = teste;
    aux->seg = NULL;
    //Procurar a posição onde a mensagem deve ficar
    if (pf->raiz == NULL) {
        // fila vazia, inserir primeira mensagem
        pf->raiz = aux;
    } else {
        prox = pf->raiz->seg;
        anterior = pf->raiz;
        while(prox != NULL){
            anterior = prox;
            prox = prox->seg;
        }
        anterior->seg = aux;
    }
    pf->tamanho++;
    return true;
}

//ordenar a lista
void ordenar_lista(struct lista *pf){
    struct no_filaS *tarefa1 = pf->raiz;
    while (tarefa1 != NULL){
        struct no_filaS *tarefa2 = tarefa1->seg;
        while (tarefa2 != NULL){
            if (tarefa1->mobile.tempoMax > tarefa2->mobile.tempoMax){ 
                int tempoMax = tarefa1->mobile.tempoMax;
                int id = tarefa1->mobile.id_tarefa;
                int tempo = tarefa1->mobile.tempo;
                int instrucoes = tarefa1->mobile.NInstrucoes;
                tarefa1->mobile.tempoMax = tarefa2->mobile.tempoMax;
                tarefa1->mobile.id_tarefa = tarefa2->mobile.id_tarefa;
                tarefa1->mobile.tempo = tarefa2->mobile.tempo;
                tarefa1->mobile.NInstrucoes = tarefa2->mobile.NInstrucoes;
                tarefa2->mobile.tempoMax = tempoMax;
                tarefa2->mobile.id_tarefa = id;
                tarefa2->mobile.tempo = tempo;
                tarefa2->mobile.NInstrucoes = instrucoes;
            }
            tarefa2 = tarefa2->seg;
        }
        tarefa1 = tarefa1->seg;
    }
}
//#####################################################################################################################################//




//################################## PROCESSO SYSTEMMANAGER #############################################################################
void SystemManager(char *filename){
    // Create shared memory
    signal(SIGUSR1,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    
    sem_unlink("MUTEX_LOGFILE");
    mutex_logFile = sem_open("MUTEX_LOGFILE", O_CREAT | O_EXCL, 0700, 1);

    sem_unlink("MUTEX_SHARED");
    mutex_Shared = sem_open("MUTEX_SHARED", O_CREAT | O_EXCL, 0700, 1);

    lerFicheiro(filename);
    sem_wait(mutex_logFile);
    logText("OFFLOAD SIMULATOR STARTING");
    sem_post(mutex_logFile);
    
    if((mqid = msgget(IPC_PRIVATE,O_CREAT|0600) ) < 0){
        perror("MESSAGE QUEUE ERROR\n");
        exit(1);
    }

    if((mkfifo(PIPENAME,O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)){
        perror("ERROR CREATING PIPE\n");
        exit(1);
    }
   
    sem_wait(mutex_Shared);
    sharedDados->pidSystem = getpid();
    sharedDados->ExecutedTasks = 0;
    sharedDados->tempoMedio = 0;
    sharedDados->NotExecutedTask = 0;
    sharedDados->termina = 0;
    inicializar(&sharedDados->list);
    sem_post(mutex_Shared);
    signal(SIGINT,HandlerSIGINT);
    signal(SIGTSTP,HandlerSIGTSTP);
    signal(SIGUSR1,HandlerURS1);
    signal(SIGUSR2,HandlerSIGURS2);
    signal(SIGTERM,HandlerSIGTERM);
    if(fork() == 0){
        taskManager();
        exit(0);
    }
    if(fork() == 0){
        monitor();
        exit(0);
    }
    if(fork()==0){
        maintenance();
        exit(0);
    }
    for(int i = 0; i < 3; i++){
        wait(NULL);
    }
  
}
//############################################################################################################################################






//############################################# TASKMANAGER  ###################################################################

void taskManager()
{   
    sem_wait(mutex_Shared);
    sharedDados->pidTask = getpid();
    sem_post(mutex_Shared);
    
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    
    sem_wait(mutex_logFile);
    logText("PROCESS TASKMANAGER CREATED");
    sem_post(mutex_logFile);

    int contador = 1;
    int fd;
    if((fd = open(PIPENAME,O_RDWR)) < 0){
        perror("Erro no pipe\n");
        exit(1);
    }
    for (int i = 0; i < sharedDados->NedgeServers; i++)
    {
        pipe(sharedDados->Server[i].pipe);
        if (fork() == 0)
        {
            createServers(i);
            exit(0);
        }
        sleep(1);   
    }
    

    pthread_t  scheduler;
    pthread_create(&scheduler,NULL,Scheduler,NULL);
    pthread_t dispatcher;
    pthread_create(&dispatcher,NULL,Dispatcher,NULL);

    while(1){
        mobile teste;
        char frase[50];
        pthread_mutex_lock(&Task);
        if(read(fd,frase,sizeof(frase))>0){
            char *token;
            token = strtok(frase," ");
        
            if(strcmp(token,"EXIT\n")==0){
                kill(sharedDados->pidSystem,SIGINT);
            }
            else if(strcmp(token,"STATS\n")==0){
                kill(sharedDados->pidSystem,SIGTSTP);
        
            } else {
                teste.NInstrucoes = atoi(token);
                token = strtok(NULL, " ");
                teste.tempoMax = atoi(token);
        
                if(sharedDados->Tasks >= sharedDados->numeroSlots){
                    sem_wait(mutex_logFile);
                    logText("TASKMANAGER: Task Eliminated...!");
                    sem_post(mutex_logFile);
                    sem_wait(mutex_Shared);
                    sharedDados->NotExecutedTask++;
                    sem_post(mutex_Shared);
            
                }else{
                    teste.id_tarefa = contador;
                    contador++;
                    teste.tempo = time(NULL);
                    sem_wait(mutex_Shared);
                    sharedDados->Tasks++;
                    colocar_lista(&sharedDados->list,teste);
                    sem_post(mutex_Shared);
                }
                THREAD = 1;
                if(sharedDados->termina == 1){
                    pthread_mutex_unlock(&Task);
                    break;
                }
                pthread_cond_signal(&Monitor_Cond);
                pthread_cond_signal(&dispatcher_worker);
                pthread_cond_signal(&Ordena);
            }
        }
        pthread_mutex_unlock(&Task);
    }
  
    for(int i = 0; i < sharedDados->NedgeServers;i++){
        wait(NULL);
    }
    pthread_join(scheduler,NULL);
    pthread_join(dispatcher,NULL);
}
//#############################################################################################################################################

//########################################  THREAD SCHEDULER    ##############################################################################
void *Scheduler(void *t){
    while(1){
        pthread_mutex_lock(&scheduler_mutex);
        while(THREAD != 1){
            pthread_cond_wait(&Ordena,&scheduler_mutex);
        }
        kill(sharedDados->pidMonitor,SIGTERM);
        struct no_filaS *pf;
        for(pf = sharedDados->list.raiz; pf != NULL; pf = pf->seg){
            int tempofinal = time(NULL);
            if((tempofinal-pf->mobile.tempo) >= pf->mobile.tempoMax){
                char buffer[50];
                sprintf(buffer,"SCHEDULER: Task with ID %d Eliminated(maxtime)",pf->mobile.id_tarefa);
                sem_wait(mutex_Shared);
                sharedDados->Tasks--;
                sharedDados->NotExecutedTask++;
                sem_post(mutex_Shared);
                sem_wait(mutex_logFile);
                logText(buffer);
                sem_post(mutex_logFile);
                retirar(&sharedDados->list,pf->mobile);
            }
        }
        
        sem_wait(mutex_Shared);
        ordenar_lista(&sharedDados->list);
        sem_post(mutex_Shared);
        
        THREAD = 0;
        if(sharedDados->termina == 1){
            pthread_exit(NULL);
        }
   
        pthread_cond_signal(&Ordena);
        pthread_cond_signal(&dispatcher_worker);
        pthread_mutex_unlock(&scheduler_mutex);
    }
}
//#############################################################################################################################






//#############################      THREAD DISPATCHER ############################################################################

void *Dispatcher(void *t){
    while(1){
        pthread_mutex_lock(&dispatcher_mutex);
        while((sharedDados->list.tamanho == 0 || verificaVCPU()==false)){
            pthread_cond_wait(&dispatcher_worker,&dispatcher_mutex);
        }
       
     
        int conta = 0;
        int posicao = -1;
		mobile pf;
        for(int i = 0; i < sharedDados->NedgeServers; i++){
        	mobile pf = sharedDados->list.raiz->mobile;
            float tempo = (float)sharedDados->Server[i].cap1 / pf.NInstrucoes;
            float tempo2 = (float)sharedDados->Server[i].cap2 / pf.NInstrucoes; 
            time_t now = time(NULL);
            
           
            if(((pf.tempoMax - (float)(now - pf.tempo) > tempo) && ((!sharedDados->Server[i].livres[0] && sharedDados->Performance == 0 && sharedDados->Server[i].manu == 0 && sharedDados->Server[i].Selecionado[0] == 0)))|| ((pf.tempoMax - (float)(now - pf.tempo) > tempo2 ) && ((!sharedDados->Server[i].livres[0] && sharedDados->Server[i].Selecionado[0] == 0) || (!sharedDados->Server[i].livres[1] && sharedDados->Server[i].Selecionado[1] == 0 ) && sharedDados->Performance == 1   && sharedDados->Server[i].manu == 0))){
                sem_wait(mutex_Shared);
                if(!sharedDados->Server[i].livres[0])
                    sharedDados->Server[i].Selecionado[0] = 1;
                else{
                    sharedDados->Server[i].Selecionado[1] = 1;
                }
                sem_post(mutex_Shared);
                char buffer[100];
                sprintf(buffer,"DISPATCHER: Task %d Selected for execution on %s",pf.id_tarefa,sharedDados->Server[i].nome);
                char buffer2[100];
                float tempoFinal;
                sprintf(buffer2,"DISPACTHER: Task %d to be send by unnamed pipe",pf.id_tarefa);
                sem_wait(mutex_logFile);
                logText(buffer);
                logText(buffer2);
                sem_post(mutex_logFile);
                sem_wait(mutex_Shared);
                write(sharedDados->Server[i].pipe[1],&pf,sizeof(mobile));
                sharedDados->Tasks--;
                
                
                retirar(&sharedDados->list,pf);
               	sem_post(mutex_Shared);
                conta++;
                posicao = i;
                break;
            }if((pf.tempoMax - (float)(now - pf.tempo) > tempo) &&(pf.tempoMax - (float)(now - pf.tempo) > tempo2)){
                conta++;
            }
        }    
     
        if(conta == 0){
            char buffer[100];
            sprintf(buffer,"DISPATCHER: Task %d Eliminated(maxtime)",pf.id_tarefa);
            sem_wait(mutex_Shared);
            sharedDados->NotExecutedTask++;
            sem_post(mutex_Shared);
            sem_wait(mutex_logFile);
            logText(buffer);
            sem_post(mutex_logFile);
            retirar(&sharedDados->list,pf);
        }

        sem_wait(mutex_Shared);
        Junta(sharedDados->taskNOT);
        sem_post(mutex_Shared);
     
        if(sharedDados->termina == 1){
            pthread_exit(NULL);
        }
       
        pthread_mutex_unlock(&dispatcher_mutex);
    }
}

//####################################################################################################################################


//############################## FUNCOES AUXILIARES ################################################################################


bool verificaVCPU(){   
    
    for(int i = 0; i < sharedDados->NedgeServers; i++){
        if(sharedDados->Performance == 0 && sharedDados->Server[i].manu == 0){
            if(sharedDados->Server[i].livres[0] == false ){
                return true;
            }
        }else if(sharedDados->Performance == 1 && sharedDados->Server[i].manu == 0){
            if((!sharedDados->Server[i].livres[0] || !sharedDados->Server[i].livres[1]) && sharedDados->Performance == 1){
                return true;
            }
        }
    }
    return false;
}

int CalculaServersFree(){  
    int contador = 0;
    for(int i = 0; i < sharedDados->NedgeServers;i++){
        if(sharedDados->Performance == 0){
            if(sharedDados->Server[i].livres[0] == false){
                contador++;
            }
        }else{
             if(sharedDados->Server[i].livres[0] == false && sharedDados->Server[i].livres[1] == false){
                 contador++;
             }

        }
  
    }
    return contador;
}


void estatisticas(){
  
    printf("\n------------- ESTATÍSTICAS ---------------\n");
    printf("Número de tarefas executadas: %d\n",sharedDados->ExecutedTasks);
    printf("Tempo médio de resposta a cada tarefa: %d\n",sharedDados->tempoMedio/sharedDados->ExecutedTasks);
    for(int i = 0; i < sharedDados->NedgeServers; i++){
        printf("Número de tarefas executadas no Server %s: %d\n",sharedDados->Server[i].nome,sharedDados->Server[i].taskExecuted);
    }
    for(int i = 0; i < sharedDados->NedgeServers; i++){
        printf("Número de operações de manutenção ao Server %s: %d\n",sharedDados->Server[i].nome,sharedDados->Server[i].ServerManutencao);
    }
    printf("Número de tarefas não executadas: %d\n",sharedDados->NotExecutedTask);
    printf("--------------------------------------------\n");
}





//#####################################################################################################################################

//#####################################    PROCESSO MONITOR ############################################################################
void monitor()
{
    sem_wait(mutex_Shared);
    sharedDados->pidMonitor = getpid();
    sem_post(mutex_Shared);
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    sem_wait(mutex_logFile);
    logText("PROCESS MONITOR CREATED");
    logText("MONITOR: NORMAL PERFORMANCE ACTIVED");
    sem_post(mutex_logFile);
    int percentagem80 =  0.8 * sharedDados->numeroSlots;
    int percentagem20 =  0.2 * sharedDados->numeroSlots;

    while(1){
        pthread_mutex_lock(&monitor_mutex);
        while(!(sharedDados->Tasks == percentagem80 && sharedDados->Performance == 0) && !(sharedDados->Tasks == percentagem20  && sharedDados->Performance == 1)){
            pthread_cond_wait(&Monitor_Cond,&monitor_mutex);
        }
    
        if(sharedDados->Tasks == percentagem80 && sharedDados->Performance == 0){
            sem_wait(mutex_logFile);
            logText("MONITOR: HIGH PERFORMANCE ACTIVED");
            sem_post(mutex_logFile);
            sem_wait(mutex_Shared);
            sharedDados->Performance = 1;
            for(int i = 0; i < sharedDados->NedgeServers; i++){
                if(sharedDados->Server[i].manu != 2){
                    sharedDados->Server[i].livres[1] = false;
                }    
            }
            sem_post(mutex_Shared);
        }
        if(sharedDados->Tasks == percentagem20  && sharedDados->Performance == 1){
            sem_wait(mutex_Shared);
            sharedDados->Performance = 0;
               for(int i = 0; i < sharedDados->NedgeServers; i++){
                sharedDados->Server[i].livres[1] = false;
            }
            sem_post(mutex_Shared);
            sem_wait(mutex_logFile);
            logText("MONITOR: HIGH PERFORMANCE DESACTIVED");
            logText("MONITOR: NORMAL PERFORMANCE ACTIVED");
            sem_post(mutex_logFile);
        }
        pthread_cond_signal(&Monitor_Cond);
        if(sharedDados->termina == 1){
            pthread_mutex_unlock(&monitor_mutex);
            break;
        }
        pthread_mutex_unlock(&monitor_mutex);
    }
}
//##################################################################################################################################


//#######################################        MAINTENANCE                ##########################################################


void maintenance()
{
    sem_wait(mutex_Shared);
    sharedDados->pidMaintenance = getpid();
    sem_post(mutex_Shared);
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    sem_wait(mutex_logFile);
    logText("PROCESS MAINTENANCE CREATED");
    sem_post(mutex_logFile);
    sleep(sharedDados->NedgeServers);
    int manutS = 0;
    int contador = 1;
    while(1){
      pthread_mutex_lock(&maintenance_mutex);
        message sender;
        message recever;
        srand(time(NULL));
        int TempoEspera =  rand() % 5 + 1;
        int TempoManu = rand() % 5 + 1;     
        int ServerManu = rand() % 3 ;           
        sender.mtype = (long)ServerManu + 1;
        sender.time = TempoManu;
        if(manutS < sharedDados->NedgeServers && !(sharedDados->Server[ServerManu].manu == 2) && sharedDados->Tasks > 0){
            manutS++;
            if(msgsnd(mqid, &sender, sizeof(message),0)<0){
               printf("ERROR TO SEND A MESSAGE\n");
            }
            (msgrcv(mqid,&recever,sizeof(recever),99,0));
        }
        sleep(TempoEspera);
        if(manutS > 0){
            for(int i = 0; i < sharedDados->NedgeServers;i++){
                message send;
                if(sharedDados->Server[i].manu == 2 && sharedDados->Server[i].done == 1){
                    send.mtype = i+1;
                    send.time = 20;
                    sem_wait(mutex_Shared);
                    sharedDados->Server[i].done = 0;
                    sem_post(mutex_Shared);
                    manutS--;
                    msgsnd(mqid,&send,sizeof(message),0);
                }
            }
        }
        if(sharedDados->termina == 1){

            pthread_mutex_unlock(&maintenance_mutex);
            break;
        }
    
        pthread_mutex_unlock(&maintenance_mutex);
    }
}
//######################################################################################################################################################




//##################################   LEITURA DE FICHEIRO  E CRIAÇÃO DA SHARED MEMORY ################################################################
void lerFicheiro(char *filename)
{
    FILE *file;
    int i = 0;
    int dat[3];
    char line[100];
    int N_server = 0;
    int count = 0;
    char *token;

    file = fopen(filename, "r");
    if (file == NULL)
    {
        sem_wait(mutex_logFile);
        logText("FILE NOT FOUND");
        sem_post(mutex_logFile);
        fclose(file);
        exit(-1);
    }

    while (i != 3)
    {
        fscanf(file, "%d", &dat[i]);
        ++i;
    }

    if (dat[2] < 2)   
    {
        sem_wait(mutex_logFile);
        logText("INVALID NUMBER OF EDGE SERVERS");
        sem_post(mutex_logFile);
        fclose(file);
        exit(-1);
    }
    int numeroSlots, MaxWait, n_server;
    numeroSlots = dat[0];
    MaxWait = dat[1];
    n_server = dat[2];
   
      if ((shmid = shmget(IPC_PRIVATE, sizeof(memory) + n_server*sizeof(edge) + sizeof(lista) + numeroSlots*sizeof(no_filaS), IPC_CREAT | 0700)) < 0)
    {
        perror("ERROR IN shmget WITH IPC_CREAT\n");
        exit(1);
    }
    if ((sharedDados = (memory *)shmat(shmid, NULL, 0)) == (memory * )-1)
    {
        perror("shmat ERROR\n");
        exit(1);
    }

    sem_wait(mutex_Shared);
    sharedDados->numeroSlots = dat[0];
    sharedDados->maxWait = dat[1];
    sharedDados->NedgeServers = dat[2];
    
    sharedDados->Tasks = 0;
    sharedDados->Performance = 0;
    sem_post(mutex_Shared);

    while (!feof(file))
    {
        char *token2;
        int contador = 0;
        token = fgets(line, 100, file);
        if (count > 0)
        {
            token2 = strtok(token, ",");
            while (token2 != NULL)
            {
                if (contador == 0)
                {
                    sem_wait(mutex_Shared);
                    strcpy(sharedDados->Server[N_server].nome,token2);
                    sem_post(mutex_Shared);
                }
                if (contador == 1)
                {
                    sem_wait(mutex_Shared);
                    sharedDados->Server[N_server].cap1 = atoi(token2);
                    
                    sem_post(mutex_Shared);
                }
                if (contador == 2)
                {
                    sem_wait(mutex_Shared);
                    sharedDados->Server[N_server].cap2 = atoi(token2);
                    sem_post(mutex_Shared);
                }
                token2 = strtok(NULL, ",");
                contador++;
            }
            
            N_server++;
        }
        count++;
    }
    fclose(file); 
}
//######################################################################################################################################################


//##############################################################   PROCESSO SERVER ####################################################################################
void createServers(int sv)
{
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);

    char buffer[80];
    

    sprintf(buffer, "%s CREATED %d %d nº %d",sharedDados->Server[sv].nome,sharedDados->Server[sv].cap1,sharedDados->Server[sv].cap2,(sv+1));
   
    sem_wait(mutex_Shared);
    
    sharedDados->Server[sv].pid = getpid();
    sharedDados->Server[sv].livres[0] = false;
    sharedDados->Server[sv].livres[1] = true;
    sharedDados->Server[sv].sig[0] = 0;
    sharedDados->Server[sv].sig[1] = 0;

    sharedDados->Server[sv].TemposVcpu[0] = 0;
    sharedDados->Server[sv].TemposVcpu[1] = 0;
    sharedDados->Server[sv].manu = 0;
    sharedDados->Server[sv].taskExecuted = 0;
    sharedDados->Server[sv].ServerManutencao = 0;
    sharedDados->Server[sv].Selecionado[0] = 0;
    sharedDados->Server[sv].Selecionado[1] = 0;
    sem_post(mutex_Shared);

    sem_wait(mutex_logFile);
    logText(buffer);
    sem_post(mutex_logFile);

    vcpu vc1;
    strcpy(vc1.nome,sharedDados->Server[sv].nome);
    vc1.NSV = sv;
    vc1.indice = 0;

    vcpu vc2;
    strcpy(vc2.nome,sharedDados->Server[sv].nome);
    vc2.NSV = sv;
    vc2.indice = 1;
    
    pthread_create(&sharedDados->Server[sv].my_thread[0], NULL,VCPU,&vc1);
    pthread_create(&sharedDados->Server[sv].my_thread[1],NULL,VCPU,&vc2);

    mobile task;
    message sender;
    message received;
    while(1){
        pthread_mutex_lock(&server_mutex);

        if(read(sharedDados->Server[sv].pipe[0],&task,sizeof(mobile))) {
            char buff[100];
            sprintf(buff, "%s: Task received!", sharedDados->Server[sv].nome);
            sem_wait(mutex_logFile);
            logText(buff); 
            sem_post(mutex_logFile);
        if (sharedDados->Server[sv].livres[0] == false) {
            sem_wait(mutex_logFile);
            logText("Task executing on VCPU1");
            sem_post(mutex_logFile);
            sem_wait(mutex_Shared);
            sharedDados->tempoMedio += (time(NULL) - task.tempo);
            sharedDados->Server[sv].livres[0] = true;
            sharedDados->Server[sv].sig[0] = 1;
            sharedDados->Server[sv].TemposVcpu[0] = (float) (sharedDados->Server[sv].cap1 / task.NInstrucoes);
            sharedDados->Server[sv].TaskExec[0] = task.id_tarefa;
            sem_post(mutex_Shared);
            pthread_cond_signal(&sharedDados->Server[sv].Vcpu[0]);
        } else if (!sharedDados->Server[sv].livres[1] && sharedDados->Performance == 1) {
            sem_wait(mutex_logFile);
            logText("Task executing on VCPU2");
            sem_post(mutex_logFile);
            sem_wait(mutex_Shared);
            sharedDados->Server[sv].livres[1] = true;
            sharedDados->Server[sv].sig[1] = 1;
            sharedDados->Server[sv].TemposVcpu[1] = (float) (sharedDados->Server[sv].cap2 / task.NInstrucoes);
            sharedDados->Server[sv].TaskExec[1] = task.id_tarefa;
            sem_post(mutex_Shared);
            pthread_cond_signal(&sharedDados->Server[sv].Vcpu[1]);
            }
        }
        if(msgrcv(mqid,&received, sizeof(received),sv+1,IPC_NOWAIT)>0){
            sem_wait(mutex_Shared);
            sharedDados->Server[sv].manu = 2;
            sem_post(mutex_Shared);
            while(sharedDados->Server[sv].livres[0] && sharedDados->Server[sv].livres[0]){
                pthread_cond_wait(&sharedDados->Server[sv].Manutencao,&server_mutex);
            }
            if(sharedDados->termina != 1){
            sender.time = received.time;
            sender.mtype = 99;
            msgsnd(mqid,&sender,sizeof(sender),0);
            char buffer[100];
            sprintf(buffer,"SERVER %s IS IN MAINTENANCE",sharedDados->Server[sv].nome);
            sem_wait(mutex_Shared);
            sharedDados->Server[sv].ServerManutencao++;
            sharedDados->Server[sv].Selecionado[0] = 2;
            sharedDados->Server[sv].Selecionado[1] = 2;
            sem_post(mutex_Shared);
            sem_wait(mutex_logFile);
            logText(buffer);
            sem_post(mutex_logFile);
            sleep(received.time);
            sem_wait(mutex_Shared);

            sharedDados->Server[sv].done = 1;
            sem_post(mutex_Shared);

            char buffer2[100];
            msgrcv(mqid,&received,sizeof(received),sv+1,0);
            sprintf(buffer2,"SERVER %s RETURNS OF MAINTENANCE",sharedDados->Server[sv].nome);
            sem_wait(mutex_Shared);
         
            if(sharedDados->Performance == 0){
                sharedDados->Server[sv].livres[0] = false;
            }else{
                sharedDados->Server[sv].livres[0] = false;
                sharedDados->Server[sv].livres[1] = false;

            }
            sharedDados->Server[sv].Selecionado[0] = 0;
            sharedDados->Server[sv].Selecionado[1] = 0;
            sharedDados->Server[sv].manu = 0;
            
            sem_post(mutex_Shared);
            sem_wait(mutex_logFile);
            logText(buffer2);
            sem_post(mutex_logFile);
            kill(sharedDados->pidTask,SIGUSR1);
            }
        }
       
       
        pthread_mutex_unlock(&server_mutex);
        }
    for (int i = 0; i < 2; i++)
    {
        pthread_join(sharedDados->Server[sv].my_thread[i],NULL);
    }
}

//#######################################        THREAD ---> REPRESENTA OS VCPU     ###################################################################
void *VCPU(void *t){
    vcpu fim = *((vcpu*)t);
    char buffer[100];
    sprintf(buffer,"VCPU %d OF EDGE SERVER %s",fim.indice + 1,fim.nome);
    sem_wait(mutex_logFile);
    logText(buffer);
    sem_post(mutex_logFile);
    while(1){
        pthread_mutex_lock(&sharedDados->Server[fim.NSV].Mvcpu[fim.indice]);
        while(sharedDados->Server[fim.NSV].sig[fim.indice] == 0){
            pthread_cond_wait(&sharedDados->Server[fim.NSV].Vcpu[fim.indice],&sharedDados->Server[fim.NSV].Mvcpu[fim.indice]);
        }
        char buffer[100];
        sprintf(buffer,"VCPU %d of server %d is working!",fim.indice + 1,fim.NSV + 1);
        sem_wait(mutex_logFile);
        logText(buffer);
        sem_post(mutex_logFile);

        sleep(sharedDados->Server[fim.NSV].TemposVcpu[fim.indice]);

        sem_wait(mutex_Shared);
        sharedDados->Server[fim.NSV].TemposVcpu[fim.indice] = 0;
        sharedDados->Server[fim.NSV].sig[fim.indice] = 0;
        sharedDados->Server[fim.NSV].Selecionado[fim.indice]= 0;
        sem_post(mutex_Shared);

        sprintf(buffer,"VCPU %d SERVER %d -> TASK %d EXECUTED!!",fim.indice + 1,fim.NSV + 1,sharedDados->Server[fim.NSV].TaskExec[fim.indice]);
        
        sem_wait(mutex_logFile);
        logText(buffer);
        sem_post(mutex_logFile);
        sem_wait(mutex_Shared);
        sharedDados->Server[fim.NSV].taskExecuted++;
        sharedDados->ExecutedTasks++;
        sharedDados->Server[fim.NSV].Selecionado[fim.indice]= 0;
        sharedDados->Server[fim.NSV].TaskExec[fim.indice] = 0;
        sharedDados->Server[fim.NSV].livres[fim.indice] = false;
        sem_post(mutex_Shared);
        pthread_cond_signal(&sharedDados->Server[fim.NSV].Manutencao);
        if(sharedDados->termina == 1){
            kill(sharedDados->pidSystem, SIGUSR2);
            pthread_exit(NULL);
        }
        kill(sharedDados->pidMonitor,SIGTERM);
        kill(sharedDados->pidTask,SIGUSR1);
        
        pthread_mutex_unlock(&sharedDados->Server[fim.NSV].Mvcpu[fim.indice]);
    }
}


//############################  FUNCAO LOG ######################################################################################################
void logText(char mensagem[])
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    FILE *f = fopen("log.txt", "a");
    printf("%d:%d:%d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, mensagem);
    fprintf(f, "%d:%d:%d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, mensagem);
    fclose(f);
}
//##################################################################################################################################################


//############################################   FUNCAO QUE DESTROI TUDO ##############################################################################3

void finalizar() {  
    sem_wait(mutex_logFile);
    logText("SIMULATOR CLOSING");
    sem_post(mutex_logFile);

    sem_close(mutex_logFile);
    sem_unlink("MUTEX_LOG");

    sem_close(mutex_Shared);
    sem_unlink("MUTEX_WRITE");

    pthread_cond_destroy(&dispatcher_worker);
    pthread_cond_destroy(&Monitor_Cond);
    pthread_cond_destroy(&destroi_cond);
    pthread_cond_destroy(&Ordena);
   
    pthread_mutex_destroy(&scheduler_mutex);
    pthread_mutex_destroy(&monitor_mutex);
    pthread_mutex_destroy(&maintenance_mutex);
    pthread_mutex_destroy(&destroi_mutex);
    pthread_mutex_destroy(&dispatcher_mutex);


    //message queue destroy
    msgctl(mqid, IPC_RMID, NULL);
    unlink(PIPENAME);
    // shared memory destroy
    shmdt(sharedDados);
    shmctl(shmid,IPC_RMID,NULL);
}
//##############################################################################################################################################
