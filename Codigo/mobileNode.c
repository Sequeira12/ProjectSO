#include "func.h"
//Bruno Sequeira (PL8) Diogo Rafael Tavares (PL7)

int main(int argc, char *argv[]) {
    if(argc != 5){
        printf("Erro na linha de comando!\n");
        exit(1);
    }

    int PEDIDOS = atoi(argv[1]);
    int IntervalosEntrePedidos =  atoi(argv[2]);
    int NInstrucoes = atoi(argv[3]);
    int TempoMaxExe = atoi(argv[4]);
    int k = 0;
    int fd;
    if((fd = open(PIPENAME,O_WRONLY))<0){
        perror("ERRO PIPE\n");
        exit(1);
    }

    while(k < PEDIDOS ){
        char envio[50];
        sprintf(envio,"%d %d",NInstrucoes,TempoMaxExe);
		write(fd,envio,sizeof(envio));
		sleep(IntervalosEntrePedidos);
        k++;
    }
    close(fd);
    return 0;
}
