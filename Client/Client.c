#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<netdb.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<fcntl.h>
#include<math.h>
#include<sys/mman.h>
#include<sys/time.h>
#include <errno.h>
#include <pthread.h>


#define PORT 8090 //Porta di default per l'inizio delle conversazioni client-server
#define SIZE_MESSAGE_BUFFER 1064 //Dimensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr //Struttura della socket
#define CODICE 25463 //Codice di utility per gestire la chiusura del server
#define CODICE2 54654 //Codice di utility per gestire l'impossibilità di aggiungere un client

void func_list(int, struct sockaddr_in, socklen_t);


struct timeval t;//Struttura per calcolare il tempo trascorso
struct sockaddr_in servaddr;// struct di supporto della socket
socklen_t len;//Lunghezza della struct della socket
char file_name[128];	//Buffer per salvare il nome del file
char buffer[SIZE_MESSAGE_BUFFER]; 	//Buffer unico per le comunicazioni
int sockfd;	//File descriptor della socket
int err;//Variabile per controllo di errore

/*
Questa funzione veien utilizzata per creare socket
Viene creata una socket e la struct di supporto
Viene ritornata la socket
*/
int create_socket(int c_port){
	// creazione della socket
	int c_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// salvo in len lunghezza della struct della socket
	len = sizeof(servaddr);
	// controllo d'errore nella creazione della socket
	if(sockfd == -1){
		perror("ATTENZIONE! Creazione della socket fallita...");
	}
	else{
		printf("Socket creata correttamente...\n");
	}
	// pulisco la memoria allocata per la struttura
	bzero(&servaddr, sizeof(servaddr));
	// setto i parametri della struttura
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	//servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	servaddr.sin_port=htons(c_port);
	// binding della socket con controllo d'errore
	return c_sockfd;
}


int main() {
	int fd;
	//Creo la socket
	sockfd=socket(AF_INET, SOCK_DGRAM, 0);
	
	//Controllo se ci sono stati errori nella creazione della socket
	if(sockfd == -1) {
		perror("Socket fallita\n");
		exit(-1);
	}
	else {
		printf("Socket creata\n");
	}
	//Catturo la lunghezza della struct della socket
	len = sizeof(servaddr);

	
	/*
	Pulisco la memoria allocata per la struttura
	La funzione bzero () cancella i dati negli n byte della memoria
    a partire dalla posizione indicata da s
	*/
	bzero(&servaddr, sizeof(len));
	//Imposto la struct
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	servaddr.sin_port=htons(PORT);
	
	//Mi presento al server
	sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
	
	//Mostro a schermo le possibili scelte
	printf("Inserisci la stringha inerente ad un comando tra: \n1) exit\n2) list\n3) download \n4) upload\n");
	
	//Pulisco buffer
	bzero(buffer, SIZE_MESSAGE_BUFFER);
	

	//Ricevo dal server la porta sulla quale connettermi
	err = recvfrom(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, &len);
	if (err < 0){
		perror("Errore nella recvfrom del main del client.");
	}
	close(sockfd);
	
	/*
	Converto la stringha ricevuta nel buffer in un intero
	atoi(char*) converte un stringha nel numero corrispondente
	*/
	int port_number =atoi(buffer);
	printf("\nNUM PORTA DOVE SONO CONNESSO %d\n",port_number);
	
	
	//Controllo se il server ha un numero massimo di client connessi, nel caso positivo riceverò come port_number un codice indicante tale evento
	if(port_number == CODICE2){
		perror("ATTENZIONE! Impossibile collegarsi al server, limite di connessioni superato.");
	}
	//Creo la socket sulla porta passata dal server
	sockfd = create_socket(port_number);
	//Ciclo infinito di richieste
	while(1){
		
		//Faccio una pulizia preliminare del buffer
		bzero(buffer, SIZE_MESSAGE_BUFFER);

		//Inserisco nel buffe rla linea di richiesta del client
		printf("\nComando:");
		fgets(buffer, SIZE_MESSAGE_BUFFER, stdin);
		//Verifico se il client vuole uscire o meno dal ciclo
		
		if(strncmp("exit", buffer, strlen("exit")) == 0){//Caso di uscita
			printf("Il client sta chiudendo la connessione...\n");
			// pulisco il buffer
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			// copio la stringa di uscita nel buffer
			strcpy(buffer, "exit");
			// invio il messaggio al server per notificargli la chiusura del client
			err = sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			if (err < 0){
				perror("Errore nell'invio del messaggio di chiusura da parte del client\n");
			}
			// chiudo la socket
			close(sockfd);
			printf("Client disconnesso.\n");
			return 0;
		}
		
		//CASO LIST
		else if (strncmp("list", buffer, strlen("list")) == 0) {
			func_list(sockfd,servaddr,len);
			
		}
		
		//CASO UPLOAD
		else if ((strncmp("upload", buffer, strlen("upload"))) == 0) {
			//Invio al server cosa voglio fare
			err = sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			/*attesa rispsta del server*/
			printf("Stai effettuando l'upload\n");
		}
		
		//CASO DOWNLOAD
		else if ((strncmp("download", buffer, strlen("download"))) == 0) {
			//Invio al server cosa voglio fare
			err = sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			/*attesa rispsta del server*/
			printf("Stai effettuando il download\n");
			
		}
		
		//CASO INPUT ERRATO
		else{
			printf("INPUT ERRATO! Inserisci un domando valido tra list, upload, download e exit.\n");
			bzero(buffer, SIZE_MESSAGE_BUFFER);
		}
	}
		return 0;
}


/*
Questa funzione viene utilizzata per richiedere una lista di file al server
Vado in attesa di ricevere questa lista dal client mediante la socket
Stampo la lista su stdout
*/
void func_list(int sockfd, struct sockaddr_in servaddr, socklen_t len){
	//Invio al server cosa voglio fare
	err = sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
	/*attesa rispsta del server*//*restituira dentro buffer la lista degliu elementi disponibili*/
	bzero(buffer,SIZE_MESSAGE_BUFFER);
	recvfrom(sockfd,buffer,sizeof(buffer),0,(SA *) &servaddr, &len);
	printf("--------------------------------\nLista dei file disponibili nel server:\n%s\n--------------------------------", buffer);
}
	