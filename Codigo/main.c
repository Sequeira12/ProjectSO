#include "func.h"
//Bruno Sequeira (PL8) Diogo Rafael Tavares (PL7)
int main(int argc, char *argv[]) {
    remove("log.txt");
    remove("TASK_PIPE");
    sigset_t mask;
    sigfillset(&mask);

    sigdelset(&mask,SIGINT);
    sigdelset(&mask,SIGTERM);
    sigdelset(&mask,SIGURG);
    sigdelset(&mask,SIGUSR1);
    sigdelset(&mask,SIGUSR2);
    sigdelset(&mask,SIGTSTP);

    sigprocmask(SIG_SETMASK,&mask, NULL);

    signal(SIGUSR1,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);

    if (argc != 2) {
        perror("INVALID INPUT ARGUMENTS");
        exit(1);
    }
    char *filename = argv[1];
    if(fork() == 0){
        SystemManager(filename);
        exit(0);
    }
    wait(NULL);
   
    return 0;
}



