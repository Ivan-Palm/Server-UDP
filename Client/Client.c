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




/*Valori definiti preliminarmente*/
#define PORT 8090 //Porta di default per l'inizio delle conversazioni client-server
#define SIZE_MESSAGE_BUFFER 1064 //Dimensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr //Struttura della socket
#define CODICE 25463 //Codice di utility per gestire la chiusura del server
#define CODICE2 54654 //Codice di utility per gestire l'impossibilità di aggiungere un client
#define SIZE_MESSAGE_BUFFER 1064 // dimensione totale del messaggio che inviamo nell'applicativo
#define SIZE_PAYLOAD 1024 // dimensione del payload nel pacchetto UDP affidabile
#define WINDOW_SIZE 3 // dimensione della finestra di spedizione

/*strutture*/
struct packet_struct{
	int counter;
	char buf[SIZE_MESSAGE_BUFFER];
	int ack;
};

/*Dichiarazioni funzioni*/
void func_list(int, struct sockaddr_in, socklen_t);



/*Variabili globali*/
struct timeval t;//Struttura per calcolare il tempo trascorso
struct sockaddr_in servaddr;// struct di supporto della socket
socklen_t len;//Lunghezza della struct della socket
char file_name[128];	//Buffer per salvare il nome del file
int packet_count;	// numero di pacchetti da inviare
char buffer[SIZE_MESSAGE_BUFFER]; 	//Buffer unico per le comunicazioni
char *buff_file_list; 	// buffer per il contenuto della lista di file
int window_base = 0; 	// parametro di posizionamento attuale nella spedizione
int sockfd;	//File descriptor della socket
int err;//Variabile per controllo di errore
int size; 	// dimensione del file da trasferire
int packet_count;	// numero di pacchetti da inviare



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
	if(sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len)==-1){
		printf("Server offline o non raggiungibile\n");
		exit(0);
	}

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
	//Controllo se il server ha un numero massimo di client connessi, nel caso positivo riceverò come port_number un codice indicante tale evento
	if(port_number==8089){
		printf("Server pieno, riprova più tardi!\n");
		exit(0);
	}
	printf("\nNUM PORTA DOVE SONO CONNESSO %d\n",port_number);
	//Mostro a schermo le possibili scelte
	printf("Inserisci un comando tra: \n1) exit\n2) list\n3) download \n4) upload\n");
	//Creo la socket sulla porta passata dal server
	sockfd = create_socket(port_number);
	//Ciclo infinito di richieste
	while(1){
		if(atoi(buffer) == CODICE){
				error("ATTENZIONE! Il server non è più in funzione.");
				return 1;
			}
		//Faccio una pulizia preliminare del buffer
		bzero(buffer, SIZE_MESSAGE_BUFFER);

		//Inserisco nel buffe rla linea di richiesta del client
		printf("\nComando:");
		fgets(buffer, SIZE_MESSAGE_BUFFER, stdin);
		//Verifico se il client vuole uscire o meno dal ciclo
		
		if((strncmp("1", buffer, strlen("1"))) == 0){//Caso di uscita
			printf("Il client sta chiudendo la connessione...\n");
			// invio il messaggio al server per notificargli la chiusura del client
			err = sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			// pulisco il buffer
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			if (err < 0){
				perror("Errore nell'invio del messaggio di chiusura da parte del client\n");
			}
			// chiudo la socket
			close(sockfd);
			printf("Client disconnesso.\n");
			return 0;
		}
		
		//CASO LIST
		else if ((strncmp("2", buffer, strlen("2"))) == 0) {
			func_list(sockfd,servaddr,len);
			
		}
		
		//CASO UPLOAD
		else if ((strncmp("4", buffer, strlen("4"))) == 0) {
			//Invio al server cosa voglio fare
			err = sendto(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			if (err < 0){
				error("Errore nella sendto nella sezione del servizio di upload del client.");
			}
			/*Attendo che il Server mi dia il permesso per proseguire*/
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			err = recvfrom(sockfd, buffer, sizeof(buffer), 0, (SA *) &servaddr, &len);
			if (err < 0){
				error("Errore nella recvfrom nella sezione del servizio di upload del client.");
			}
			/*Scelgo il file da inviare*/
			int file,dim;
			int counter=0;
			int seq = 0;
			file=open("lista_c.txt",O_RDONLY,0666);
			if(open==-1){
				printf("Errore apertura file\n");
				return;
			}
			if(file<0){printf("Errore apertura lista dei file");}
			/*Vedo la lunghezza complessiva del file*/
			size = lseek(file,0,SEEK_END);//Scorro la testina dall'inzio alla fine
			buff_file_list = malloc(size);//Alloco tanta memoria per contenerlo
			lseek(file,0,0);//riposiziono la testina all'inzio
			/*Lettura dei dati*/
			read(file,buff_file_list, size);
			printf("Lista dei file: \n%s\n",buff_file_list);
			/*Scelta del file*/
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			SCELTA:
			printf("Scegli il file:");
			fgets(buffer, SIZE_MESSAGE_BUFFER,stdin);
			close(file);
			bzero(file_name,128);//pulisco il buffer contenente il nome del file
			strncpy(file_name,buffer,strlen(buffer)-1);
			
			
			/*Apro il file da inviare per leggere i suoi dati*/
			int file_inv = open(file_name,O_RDONLY,0666);
			if(file_inv<0){
				printf("File non presente nella directory\n");
				bzero(file_name,128);//pulisco il buffer contenente il nome del file
				bzero(buffer, SIZE_MESSAGE_BUFFER);
				goto SCELTA;
			}

			bzero(buffer, SIZE_MESSAGE_BUFFER);
			printf("Sto inviando al server il nome %s\n",file_name);
			
			
			/*Invio il nome del file al server*/
			sendto(sockfd, file_name, sizeof(file_name), 0, (SA *) &servaddr, len);
			if (err < 0){
				error("Errore nella sendto della get_name_and_size_file del client.");
			}
			
			
			/*Calcolo quanti pacchetti devo inviare al sevrer*/
			dim = lseek(file_inv, 0, SEEK_END);
			packet_count = (ceil((dim/SIZE_PAYLOAD)))+1;
			printf("Numero di pacchetti da caricare: %d.\n", packet_count);
			lseek(file_inv, 0, 0);
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			/*Inserisco la dimensione effettiva delf ile nel buffer e la mando al server */
			sprintf(buffer, "%d", size);
			/*Invio la dimensione del file*/
			if (sendto(sockfd, buffer, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, len) <0){
				error("Errore nella sento della send_len_file del client.");
			}			
			
			/*Inizio il caricamento del file*/
			struct packet_struct file_struct[packet_count];//creo tanti pacchetti quanti calcolati prima con packet_count
			
			/*Creo un buffer avente la dimensione pari ad un pacchetto*/
			char *temp_buf;
			temp_buf = malloc(SIZE_PAYLOAD);//temp_buf ha la dimensione di un pacchetto
			for (int i = 0; i < packet_count; i++){
				bzero(temp_buf, SIZE_PAYLOAD);//pulisco tempo_buf
				read(file_inv, temp_buf, SIZE_PAYLOAD);//leggo quanto possibile da incapsulare in un pacchetto
				/*Creo un array di dimensione pari ad un pacchetto*/
				char pacchetto[SIZE_MESSAGE_BUFFER];
				/*Inserisco i dati di quel pacchetto*/
				sprintf(pacchetto, "%d ", i);//Copio il numero di sequenza di quel pacchetto
				strcat(pacchetto, temp_buf);//Copio nell'array creato per contenerlo, il numero di sequenza ed il contenuto precedentemente ricavato
				sprintf(file_struct[i].buf, "%s", pacchetto);//Riscrivo quello appena creato nella struttura nella posizione i-esima
				
				file_struct[i].counter = i;//assegno l'indice i-esimo all'entry in quella struttura
			/*attesa rispsta del server*/
			printf("Stai effettuando l'upload\n");
			}
			/*Stampo l'inter struttura*/
			for (int i = 0; i < packet_count; i++){
				printf("\nFILE_STRUCT[%d].BUF contiene : -----------------------------------\n%s\n-------------------------------\n",i,file_struct[i].buf);
			}
			/*Da qui in poi ho tutti i pacchetti salvati nella struttura*/
			
			printf("Ho caricato i pacchetti nella struttura\n");
			/*Fase di invio dei pacchetti*/
			/*Vedo quanti pacchetti non sono multipli della windows size*/
			int offset = packet_count%WINDOW_SIZE;
			/*Caso in cui il numero dei pacchetti da inviare in maniera diversa perche non sono un multipli della WINDOWS_SIZE*/
			if(offset > 0){//Numoero di pacchetti da inviare "diversamente"
			/*Se ci sono pacchetti "normali" da inviare invio quelli*/
				if(packet_count-seq >= offset){
					printf("Inizio inviare i pack normali\n");
					while(seq<packet_count - offset){
						printf("packet_count= %d\t\t seq= %d\t\t  di cui offset= %d\t\t\n", packet_count, seq, offset);
						seq = send_packet_GO_BACK_N(file_struct, seq, WINDOW_SIZE);//mando la struttura contenente i pacchetti, la sequenza, e la dimensione della finestra
					}
				}
				printf("Ho finito di inviare i pack normali\n");
				printf("Inizio inviare i pack diversi\n");
				/*Una volta inviati i pacchetti "normali" invio offset pacchetti "diversi"*/
				printf("packet_count= %d\t\t seq= %d\t\t offset= %d\t\t.\n", packet_count, seq, offset);
				seq = send_packet_GO_BACK_N(file_struct, seq, offset);//mando la struttura contenente i pacchetti, la sequenza, e il numero di pacchetti rimanenti
				printf("Ho finito di inviare i pack diversi\n");
			}
			/*Caso in cui il numero dei pacchetti è un multiplo della WINDOWS_SIZE*/
			else{
				while(seq < packet_count){
					seq = send_packet_GO_BACK_N(file_struct, seq, WINDOW_SIZE);//mando la struttura contenente i pacchetti, la sequenza, e la dimensione della finestra
				}
			}
		}
		
		//CASO DOWNLOAD
		else if((strncmp("3", buffer, strlen("3"))) == 0) {
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
	
	
/*offset ha il valore della WINDOWS SIZE per i pacchetti "normali" e offset per i pack "diversi"*/
int send_packet_GO_BACK_N(struct packet_struct *file_struct, int seq, int offset){
	/*Ciclo for che invia WINDOWS_SIZE pacchetti alla volta*/
	int lock=0;
	for(int i = 0; i < offset; i++){	
		//imposto l'ack dei N pacchetti che sto inviando come 0, lo metterò a 1 una volta ricevuto l'ack complessivo dal client
		for(int h=0;h<offset;h++){
			/*seq(inzialmente uguale a 0, indica il numero del pack)*/
			file_struct[seq+h].ack = 0;
		}
		/*Mando i primi N pacchetti*/
		for(int h=0;h<offset;h++){
			if(sendto(sockfd, file_struct[seq+h].buf, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, len) < 0) { 
				fprintf(stderr, "Errore nell'invio del pacchetto numero: %d.\n", seq);
				exit(EXIT_FAILURE);
			}
		}
		printf("Pacchetti [%d, %d, %d] inviati attendo il riscontro\n",seq,seq+1,seq+2);
		// pulisco il buffer
		bzero(buffer, SIZE_MESSAGE_BUFFER);
		printf("Attendo ACK accoumulativo di [%d, %d, %d]  \n",seq,seq+1,seq+2);
		//Ricevo ack
		int err = recvfrom(sockfd,buffer, SIZE_MESSAGE_BUFFER, 0, (SA *) &servaddr, &len);
		if (err < 0){
			if(errno == EAGAIN)
			{
				lock = 1;
				printf("il pacchetto è andato perso, ack: %d non ricevuto\n",seq);
			}
			else{
				error("Errore nella recvfrom della send_packet del server.");
			}
		}
		if(lock == 0){
			printf("Ho ricevuto l'ack dei pacchetti [%d, %d, %d]  \n",seq,seq+1,seq+2);
			// è una variabile che assume il valore del numero ricevuto 
			// con l'ack, che è proprio il numero corrispondente al pacchetto, 
			// ora imposto l'ack a 1
			int check = atoi(buffer);
			if(check >= 0){
				file_struct[check].ack = 1;
			}
		}
		seq=seq+offset;
	}
	/*Controllo se ho inviato tutti i pacchetti*/
	/*printf("Sto entrando nella check per l'eventuale ritrasmissione di pacchetti persi seq = %d.\n",seq);
	// checking degli ACK della finestra
	for(int i = window_base; i < (window_base + offset); i++){
		if(file_struct[i].ack!=1){
			//invio di nuovo il pacchetto
			RESEND:	
			if(sendto(sockfd, file_struct[i].buf, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, len) < 0) { 
				fprintf(stderr, "Errore nell'invio del pacchetto numero: %d.\n", i);
				exit(EXIT_FAILURE);
			}	
			printf("Sto inviando nuovamente il pacchetto %d.\n", i);
			// pulisco il buffer
			bzero(buffer, SIZE_MESSAGE_BUFFER);
			// ricevo il messaggio dal client con l'ack del pacchetto ricevuto
			if(recvfrom(sockfd, buffer, SIZE_MESSAGE_BUFFER, 0, (struct sockaddr *) &servaddr, &len) < 0){
				if(errno == EAGAIN)
				{
					if(i==packet_count-1)
					{
						goto SENDEND;
					}
					usleep(50);
					goto RESEND;
				}
				else
				{
					error("errore nella recv from della check_packet_sended_of_window");
				}
			}
			SENDEND:
			// setto l'ACK del pacchetto ricevuto correttamente
			printf("Ho ricevuto l'ack del pacchetto ritrasmesso %s.\n", buffer);
			int check;
			check = atoi(buffer);
			if(check > 0){
				file_struct[check].ack = 1;
			}
		}
	}*/
	
return seq;
}