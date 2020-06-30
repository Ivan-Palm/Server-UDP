#include<stdio.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/time.h>
#include<signal.h>
#include<math.h>
#include<sys/mman.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<pthread.h>
#include <errno.h>


#define PORT 8090 // porta di default per l'inizio delle conversazioni client-server
#define MAXLINE 1024
#define MAX_CONNECTION 5 // numero massimo di connessioni accettate dal server
#define SIZE_MESSAGE_BUFFER 1024 // diensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr // struttura della socket

void func_exit(int , int , pid_t );
void func_list(int, struct sockaddr_in, socklen_t);

char *buff_file_list; 	// buffer per il contenuto della lista di file
char buffer[SIZE_MESSAGE_BUFFER]; 	// buffer unico per le comunicazioni
int s_sockfd;	// file descriptor della socket usata dai processi figli
pid_t parent_pid; 	// pid del primo processo padre nel main
int num_client=0;
int size; 	// dimensione del file da trasferire
struct sockaddr_in servaddr;	// struct di supporto della socket
socklen_t len;	// lunghezza della struct della socket
int port_number = 0; 	// variabile di utility per il calcolo delle porte successive da dare al client
int shmid; 	// identificativo della memoria condivisa
int client_port; 	// porta che diamo al client per le successive trasmissioni multiprocesso
int sockfd;	// file descriptor di socket



int create_socket(int s_port){
	// creazione della socket
	int s_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// salvo in len la lunghezza della struct della socket
	len = sizeof(servaddr);
	// controllo d'errore nella creazione della socket
	if(s_sockfd == -1){
		herror("ATTENZIONE! Creazione della socket fallita...");
	}
	else{
		printf("Socket creata correttamente...\n");
	}
	// pulisco la memoria allocata per la struttura della socket
	bzero(&servaddr,sizeof(servaddr));
	// setto i parametri della struttura della socket
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	//servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	servaddr.sin_port=htons(s_port);

	// binding della socket con controllo d'errore
	if((bind(s_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))!=0){
		herror("ATTENZIONE! Binding della socket fallito...");
	}
	else{
		printf("Socket-Binding eseguito correttamente...\n");
	}
	return s_sockfd;
}


int main(){
	//inizializzo la sharedmemory per salvare i process-id dei child
	shmid = shmget(IPC_PRIVATE, sizeof(int)*MAX_CONNECTION, IPC_CREAT|0666);
	if(shmid == -1){
		herror("Errore nella shmget nel main del server.");
	}
	
	//salvo il pid del processo padre in una variabile globale
	parent_pid = getpid();
	//creo la socket di comunicazione
	s_sockfd = create_socket(PORT);
	
	//creo un processo che gestisce l'eventuale richiesta di chiusura del server
	pid_t pid = fork();
	if(pid == 0){
		signal(SIGUSR1, SIG_IGN);
		//child_exit(shmid);
	}
	
	//entro nel ciclo infinito di accoglienza di richieste
	while(1){
		
		bzero(buffer, SIZE_MESSAGE_BUFFER);
		//attendo un client
		if(recvfrom(s_sockfd, buffer, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, &len) < 0){
			herror("Errore nella recvfrom nel primo while del server.");
		}
		bzero(buffer, SIZE_MESSAGE_BUFFER);
		//aumento contatore che segnala i client attivi
		num_client = num_client + 1;
		
		//verifico se ho superato il range di client ammissibili
		if(num_client>=MAX_CONNECTION){
			printf("Numero massimo di client raggiunto, ne elimino uno\n");
			num_client-=1;
		}
		else{
			//aggiorno il numero di porta sulla quale fare connettere i client
			port_number = port_number + 1;
			//aggiorno numero di porta da passare al client
			client_port = PORT + port_number;//il primo avra 8091
			printf("\n------------------------------NUOVO UTENTE CONNESSO!Client port %d------------------------------\n",client_port);
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			//scrivo il valore aggionrato nel buffer di comunicazione
			sprintf(buffer,"%d",client_port);
			if(sendto(s_sockfd, buffer, SIZE_MESSAGE_BUFFER,0, (struct sockaddr *) &servaddr, len) < 0){
				herror("Errore nella sendto 2 del primo while del main del server.");
			}
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			//creo un child per ogni connessione, esso la gestirà mentre il padre rimarrà in ascolto di nuove eventuali connessioni
			pid_t pid = fork();
			if(pid == 0){
				RESTART_SOCKET:
				//Creo una nuova socket per questa connessione e chiudo la socket di comunicazione del padre
				sockfd = create_socket(client_port);
				close(s_sockfd);
				//entro nel ciclo di ascolto infinito
				while(1){
					bzero(buffer, SIZE_MESSAGE_BUFFER);
					//aspetto di ricevere un messaggio
					if(recvfrom(sockfd, buffer, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, &len) < 0){
						if (errno==EAGAIN)
						{
							goto RESTART_SOCKET;
						}
						else{
							herror("Errore nella recvfrom del secondo while del main del server.");
						}
					}
					/*Gestisco la richiesta del client*/
					if(strncmp("exit", buffer, strlen("exit")) == 0){
						printf("Client port %d -> Richiesto exit\n",client_port);
						func_exit(client_port,sockfd,parent_pid);
						while(1){
							sleep(1000);
						}
					}
					else if(strncmp("list", buffer, strlen("list")) == 0){
						printf("Client port %d -> Richiesto list\n",client_port);
						func_list(sockfd,servaddr,len);
					}
					else if(strncmp("download", buffer, strlen("download")) == 0){
						printf("Client port %d -> Richiesto download\n",client_port);
						//func_download(sockfd,servaddr,len);
						
						
					}	
					else if(strncmp("upload", buffer, strlen("upload")) == 0){
						printf("Client port %d -> Richiesto upload\n",client_port);
						//func_upload(sockfd,servaddr,len);
							
					}
					else{
						//func_error(sockfd,servaddr,len);
					}
				}
				
			}	
		}
	}
	return 0;
}

void func_list(int sockfd, struct sockaddr_in servaddr, socklen_t len){
	int ret;
	int fd; //Puntatore al file contenente la lista dei file
	fd= open("lista.txt",O_RDONLY,0666);//apro uno stream di sola lettura verso il file
	if(fd==-1){
		printf("Errore apertura lista dei file\n");
		return;
	}
	size = lseek(fd,0,SEEK_END); //Vedo la dimensione del file
	if(size<0){
		printf("Errore lettura della dimensione della lista dei file\n");
		return;
	}
	buff_file_list=malloc(size); //alloco la memoria per contenerlo
	if(buff_file_list==NULL){
		printf("Errore allocazione memoria per contenere la lista dei file\n");
		return;
	}
	lseek(fd,0,0);//riposiziono la testina all'inizio del file
	while((read(fd,buff_file_list,size)==-1)){//inserisco all'interno di buff_file_list l'intero contenuto del file
		if(errno!=EINTR){
			printf("Errore lettura contenuto della lista dei file\n");
			return;
		}
	}
	while((sendto(sockfd,buff_file_list,size,0,(struct sockaddr *) &servaddr, len))==-1){//metto il contenuto sulla socket
		if(errno!=EINTR){
		printf("Errore caricamneot lista dei file sulla socket\n");
		return;
		}
	}	
	free(buff_file_list);//dealloco la memoria allocata precedentemente con la malloc
}

void func_exit(int client_port, int socket_fd, pid_t pid){
	printf("Chiudo la connessione verso la porta: %d.\n", client_port);
	int ret = close(socket_fd);
	if(ret == -1){
		error("Errore nella chiusura della socket verso la porta.",client_port);
	}
	kill(pid, SIGUSR1);
}
	
		
		