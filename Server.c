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


char buffer[SIZE_MESSAGE_BUFFER]; 	// buffer unico per le comunicazioni
int s_sockfd;	// file descriptor della socket usata dai processi figli
pid_t parent_pid; 	// pid del primo processo padre nel main
int num_client=0;
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
		error("ATTENZIONE! Creazione della socket fallita...");
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
		error("ATTENZIONE! Binding della socket fallito...");
	}
	else{
		printf("Socket-Binding eseguito correttamente...\n");
	}
	return s_sockfd;
}


void main(){
	//inizializzo la sharedmemory per salvare i process-id dei child
	shmid = shmget(IPC_PRIVATE, sizeof(int)*MAX_CONNECTION, IPC_CREAT|0666);
	if(shmid == -1){
		error("Errore nella shmget nel main del server.");
	}
	
	//salvo il pid del processo padre in una variabile globale
	parent_pid = getpid();
	
	s_sockfd = create_socket(PORT);
	
	//creo un processo che gestisce l'eventuale richiesta di chiusura del server
	pid_t pid = fork();
	if(pid == 0){
		signal(SIGUSR1, SIG_IGN);
		//child_exit(shmid);
	}
	
	//entro nel ciclo infinito di accoglienza di richieste
	while(1){
		//attendo un client
		if(recvfrom(s_sockfd, buffer, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, &len) < 0){
			error("Errore nella recvfrom nel primo while del server.");
		}
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
			client_port = PORT + port_number;
			//scrivo il valore aggionrato nel buffer di comunicazione
			sprintf(buffer,"NUM PORT:%d",client_port);
			if(sendto(s_sockfd, buffer, SIZE_MESSAGE_BUFFER,0, (struct sockaddr *) &servaddr, len) < 0){
				error("Errore nella sendto 2 del primo while del main del server.");
			}
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			//creo un child per ogni connessione, esso la gestirà mentre il padre rimarrà in ascolto di nuove eventuali connessioni
			pid_t pid = fork();
			if(pid == 0){
				//Creo una nuova socket per questa connessione e chiudo la socket di comunicazione del padre
				sockfd = create_socket(client_port);
				close(s_sockfd);
				//entro nel ciclo di ascolto infinito
				while(1){
					// pulisco il buffer
					bzero(buffer, SIZE_MESSAGE_BUFFER);
					if(recvfrom(sockfd, buffer, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, &len) < 0){
							error("Errore nella recvfrom del secondo while del main del server.");
					}
					
					/*Gestisco la richiesta del client*/
					if(strncmp("exit", buffer, strlen("exit")) == 0){
						sprintf(buffer,"FUNZIONE EXIT");
						if(sendto(s_sockfd, buffer, SIZE_MESSAGE_BUFFER,0, (struct sockaddr *) &servaddr, len) < 0){
							error("Errore nella risposta EXIT");
						}
						bzero(buffer, SIZE_MESSAGE_BUFFER);
					}
	
					else if(strncmp("list", buffer, strlen("list")) == 0){
						sprintf(buffer,"FUNZIONE LIST");
						if(sendto(s_sockfd, buffer, SIZE_MESSAGE_BUFFER,0, (struct sockaddr *) &servaddr, len) < 0){
							error("Errore nella risposta LIST");
						}
						bzero(buffer, SIZE_MESSAGE_BUFFER);
					}
	
					else if(strncmp("download", buffer, strlen("download")) == 0){
						sprintf(buffer,"FUNZIONE DOWNLOAD");
						if(sendto(s_sockfd, buffer, SIZE_MESSAGE_BUFFER,0, (struct sockaddr *) &servaddr, len) < 0){
							error("Errore nella risposta DOWNLOAD");
						}
						bzero(buffer, SIZE_MESSAGE_BUFFER);
					}	
	
					else if(strncmp("upload", buffer, strlen("upload")) == 0){
						sprintf(buffer,"FUNZIONE UPLOAD");
						if(sendto(s_sockfd, buffer, SIZE_MESSAGE_BUFFER,0, (struct sockaddr *) &servaddr, len) < 0){
							error("Errore nella risposta UPLOAD");
						}
						bzero(buffer, SIZE_MESSAGE_BUFFER);
					}

					else{
						sprintf(buffer,"HAI SBAGLIATO");
					
					}
				}
				return;
			}	
		}
	}
}
		
		