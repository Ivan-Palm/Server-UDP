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
#define DIMENSIONE_MESSAGGI 1064 //Dimensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr //Struttura della socket
#define CODICE 25463 //Codice di utility per gestire la chiusura del server
#define CODICE2 54654 //Codice di utility per gestire l'impossibilità di aggiungere un client
#define DIMENSIONE_PACCHETTO 1024 // dimensione del payload nel pacchetto UDP affidabile
#define DIMENSIONE_FINESTRA 3 // dimensione della finestra di spedizione
#define SEND_FILE_TIMEOUT 100000 // timeout di invio
#define CONNECTION_TIMER 1000000 //timeout di connessione

/*strutture*/
struct inside_the_package{
	int numero_ordine;
	char buf[DIMENSIONE_MESSAGGI];
	int ack;
};

/*Dichiarazioni funzioni*/
void lista_dei_file(int, struct sockaddr_in, socklen_t);
int creazione_socket(int);
void setTimeout(double,int);




/*Variabili globali*/
int si; //utile per il riscontro dei pack
int num=0; //per vedere i pack fuori ordine
struct timers t;//Struttura per calcolare il tempo trascorso
struct sockaddr_in servaddr;// struct di supporto della socket
socklen_t len;//Lunghezza della struct della socket
char file_name[128];	//Buffer per salvare il nome del file
int num_pacchetti;	// numero di pacchetti da inviare
char buffer[DIMENSIONE_MESSAGGI]; 	//Buffer unico per le comunicazioni
char *lista_dei_files; 	// buffer per il contenuto della lista di file
int window_base = 0; 	// parametro di posizionamento attuale nella spedizione
int socketone;	//File descriptor della socket
int err;//Variabile per controllo di errore
int dimensione; 	// dimensione del file da trasferire
int num_pacchetti;	// numero di pacchetti da inviare
int id=0;


int main() {
	int fd;
	//Creo la socket
	socketone=socket(AF_INET, SOCK_DGRAM, 0);
	
	//Controllo se ci sono stati errori nella creazione della socket
	if(socketone == -1) {
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
	sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
	
	/**timer per la scadenza della richiesta**/

	//Pulisco buffer
	bzero(buffer, DIMENSIONE_MESSAGGI);
	

	//Ricevo dal server la porta sulla quale connettermi
	err = recvfrom(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, &len);
	if (err < 0){
		perror("Errore nella recvfrom del main del client."); 
		printf("Il server non è momentanemanete raggiungibile\n");
		printf("Sto chiudendo la socket\n");
		close(socketone);
		exit(0);
	}
	close(socketone);
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
	socketone = creazione_socket(port_number);
	//Ciclo infinito di richieste
	while(1){
		if(atoi(buffer) == CODICE){
				perror("ATTENZIONE! Il server non è più in funzione.");
				return 1;
			}
		//Faccio una pulizia preliminare del buffer
		bzero(buffer, DIMENSIONE_MESSAGGI);

		//Inserisco nel buffe rla linea di richiesta del client
		printf("\nComando:");
		fgets(buffer, DIMENSIONE_MESSAGGI, stdin);
		//Verifico se il client vuole uscire o meno dal ciclo
		if((strncmp("1", buffer, strlen("1"))) == 0){//Caso di uscita
			printf("Il client sta chiudendo la connessione...\n");
			// invio il messaggio al server per notificargli la chiusura del client
			err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			// pulisco il buffer
			bzero(buffer, DIMENSIONE_MESSAGGI);
			if (err < 0){
				perror("Errore nell'invio del messaggio di chiusura da parte del client\n");
			}
			// chiudo la socket
			close(socketone);
			printf("Client disconnesso.\n");
			return 0;
		}
		
		//CASO LIST
		else if ((strncmp("2", buffer, strlen("2"))) == 0) {
			lista_dei_file(socketone,servaddr,len);
			
		}
		
		//CASO UPLOAD
		else if ((strncmp("4", buffer, strlen("4"))) == 0) {
			//Invio al server l'azione che voglio fare
			id=0;
			err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			if (err < 0){
				perror("Errore nella sendto nella sezione del servizio di upload del client.");
			}
			printf("------------------------------------------------[FASE DI UPLOAD]------------------------------------------------\n");
			/*Attendo che il Server mi dia il permesso per proseguire*/
			bzero(buffer, DIMENSIONE_MESSAGGI);
			err = recvfrom(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, &len);
			if (err < 0){
				perror("Errore nella recvfrom nella sezione del servizio di upload del client.");
			}
			/*
			Scelgo il file da inviare
			Per farlo apro uno stream di sola lettura verso il file presente nella directory del client, 
			dove all'interno ci sarà tutta la lista dei file caricabili sul server
			*/
			int file,dim;
			int numero_ordine=0;
			int seq = 0;
			file=open("lista_c.txt",O_RDONLY,0666);
			if(open==NULL){
				printf("Errore apertura file\n");
				return 0;
			}
			if(file<0){printf("Errore apertura lista dei file");}
			/*
			Calcolo la lunghezza complessiva del file facendo scorrere la testina dall'inzio alla fine
			lseek ritornerà l'intera grandezza del file
			*/
			dimensione = lseek(file,0,SEEK_END);//Scorro la testina dall'inzio alla fine
			lista_dei_files = malloc(dimensione);//Alloco tanta memoria per contenerlo
			lseek(file,0,0);//riposiziono la testina all'inzio del file(meotodo di accesso diretto)
			
			/*Lettura dei della lista dei file caricabili sul server*/
			read(file,lista_dei_files, dimensione);
			printf("Lista dei file: \n%s\n",lista_dei_files);
			
			/*Scelta del file*/
			bzero(buffer, DIMENSIONE_MESSAGGI);
			SCELTA:
			printf("Scegli il file inserendo il suo nome per intero(compreso di .txt):\t");
			fgets(buffer, DIMENSIONE_MESSAGGI,stdin);// catturo la stringa passata come parametro da standard input
			close(file);//chiudo lo stream verso il file
			bzero(file_name,128);//pulisco il buffer contenente il nome del file
			strncpy(file_name,buffer,strlen(buffer)-1);//copio il nome del file in un puntatore a char
			
			
			/*Apro il file da inviare per leggere i suoi dati*/
			int file_inv = open(file_name,O_RDONLY,0666);
			/*Controllo se il file passato è presente nella lista dei file*/
			if(file_inv<0){
				printf("File non presente nella directory\n");
				bzero(file_name,128);//pulisco il buffer contenente il nome del file
				bzero(buffer, DIMENSIONE_MESSAGGI);
				goto SCELTA;//se non è presente do la possibilità di sceglierlo di nuovo
			}

			bzero(buffer, DIMENSIONE_MESSAGGI);
			printf("Sto inviando al server il nome %s\n",file_name);//controllo sul nome del file
			
			
			/*Invio il nome del file al server*/
			sendto(socketone, file_name, sizeof(file_name), 0, (SA *) &servaddr, len);
			if (err < 0){
				perror("Errore nella sendto della get_name_and_size_file del client.");
			}
			
			printf("------------------------------------------------[INFORMAZIONI PRELIMINARI]------------------------------------------------\n");
			/*Calcolo quanti pacchetti devo inviare al server*/
			dim = lseek(file_inv, 0, SEEK_END);
			num_pacchetti = (ceil((dim/DIMENSIONE_PACCHETTO)))+1;
			printf("Numero di pacchetti da caricare: %d.\n", num_pacchetti);
			lseek(file_inv, 0, 0);
			bzero(buffer, DIMENSIONE_MESSAGGI);
			
			
			/*Inserisco la dimensione effettiva del file nel buffer e la mando al server */
			sprintf(buffer, "%d", dim);
			
			/*Invio la dimensione del file*/
			if (sendto(socketone, buffer, DIMENSIONE_MESSAGGI, 0, (struct sockaddr *) &servaddr, len) <0){
				perror("Errore nella sento della send_len_file del client.");
			}			
			
			/*Inizio il caricamento del file*/
			struct inside_the_package file_struct[num_pacchetti];//creo tanti pacchetti quanti calcolati prima con num_pacchetti
			
			/*Creo un buffer avente la dimensione pari ad un pacchetto*/
			char *temp_buf;
			temp_buf = malloc(DIMENSIONE_PACCHETTO);//temp_buf ha la dimensione di un pacchetto
			for (int i = 0; i < num_pacchetti; i++){//ciclo for ripetuto per tutti i pacchetti
			
				bzero(temp_buf, DIMENSIONE_PACCHETTO);//pulisco tempo_buf
				read(file_inv, temp_buf, DIMENSIONE_PACCHETTO);//leggo quanto possibile da incapsulare in un pacchetto
				
				/*Creo un array di dimensione pari ad un pacchetto*/
				char pacchetto[DIMENSIONE_MESSAGGI];
				
				/*Inserisco i dati di quel pacchetto*/
				sprintf(pacchetto, "%d ", i);//Copio il numero di sequenza di quel pacchetto
				strcat(pacchetto, temp_buf);//Copio nell'array creato per contenerlo, il numero di sequenza ed il contenuto precedentemente ricavato
				sprintf(file_struct[i].buf, "%s", pacchetto);//Riscrivo quello appena creato nella struttura nella posizione i-esima
				
				file_struct[i].numero_ordine = i;//assegno l'indice i-esimo all'entry in quella struttura
				
			
			}
			/*Stampo l'inter struttura*/
			for (int i = 0; i < num_pacchetti; i++){
				printf("\nFILE_STRUCT[%d].BUF contiene :\n%s\n--------------------------------------------------------------\n",i,file_struct[i].buf);
			}
			
			/*Da qui in poi ho tutti i pacchetti salvati nella struttura*/
			printf("Ho caricato i pacchetti nella struttura\n");
			printf("------------------------------------------------[FASE DI SCAMBIO]------------------------------------------------\n");
			/*Fase di invio dei pacchetti*/
			
			/*Vedo quanti pacchetti non sono multipli della windows dimensione*/
			int offset = num_pacchetti%DIMENSIONE_FINESTRA;
			/*Caso in cui il numero dei pacchetti da inviare in maniera diversa perche non sono un multipli della WINDOWS_SIZE*/
			

			if(offset > 0){//Numoero di pacchetti da inviare "multipli di WINDOWS_SIZE"*/
			/*Se ci sono pacchetti "multipli di WINDOWS_SIZE" da inviare invio quelli*/
				if(num_pacchetti-seq >= offset){
					while(seq<num_pacchetti - offset){//fino a quando non sono arrivato al primo pack "NON multiplo di WINDOWS_SIZE"
						printf("num_pacchetti= %d\t\t seq= %d\t\t  di cui offset= %d\t\t\n", num_pacchetti, seq, offset);
						seq = send_packet_GO_BACK_N(file_struct, seq, DIMENSIONE_FINESTRA,num);//mando la struttura contenente i pacchetti, la sequenza, e la dimensione della finestra
				
					}
				}
		
				/*Una volta inviati i pacchetti "multipli di WINDOWS_SIZE" invio offset pacchetti "NON multipli di WINDOWS_SIZE"*/
				printf("num_pacchetti= %d\t\t seq= %d\t\t offset= %d\t\t.\n", num_pacchetti, seq, offset);
				
				/*Controllo se ho finito di mandare gia i pacchetti a causa di ritrasmissioni*/
				if(seq>=num_pacchetti){
						printf("------------------------------------------------[FINE FASE DI UPLOAD]------------------------------------------------\n");
						goto FINISH;
				}
				/*Nel caso ne rimangano alcuni, li invio*/
				seq = send_packet_GO_BACK_N(file_struct, seq, offset);//mando la struttura contenente i pacchetti, la sequenza, e il numero di pacchetti rimanenti
				
			}
			/*Caso in cui il numero dei pacchetti è un multiplo della WINDOWS_SIZE*/
			else{
				while(seq < num_pacchetti){
					seq = send_packet_GO_BACK_N(file_struct, seq, DIMENSIONE_FINESTRA);//mando la struttura contenente i pacchetti, la sequenza, e la dimensione della finestra
				}
			}
			FINISH:
			/*Reset delle informaizoni*/
			si=0;
			seq=0;
			num=0;
			bzero(buffer, DIMENSIONE_MESSAGGI);
		}
		
		//CASO DOWNLOAD
		else if((strncmp("3", buffer, strlen("3"))) == 0) {
			//Invio al server cosa voglio fare
			err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			/*attesa rispsta del server*/
			printf("Stai effettuando il download\n");
			
		}
		
		//CASO INPUT ERRATO
		else{
			printf("INPUT ERRATO! Inserisci un domando valido tra list, upload, download e exit.\n");
			bzero(buffer, DIMENSIONE_MESSAGGI);
		}
	}
		return 0;
}


/*
Questa funzione veien utilizzata per creare socket
Viene creata una socket e la struct di supporto
Viene ritornata la socket
*/
int creazione_socket(int c_port){
	// creazione della socket
	int c_socketone = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// salvo in len lunghezza della struct della socket
	len = sizeof(servaddr);
	// controllo d'errore nella creazione della socket
	if(socketone == -1){
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
	return c_socketone;
}


/*
Questa funzione viene utilizzata per richiedere una lista di file al server
Vado in attesa di ricevere questa lista dal client mediante la socket
Stampo la lista su stdout
*/
void lista_dei_file(int socketone, struct sockaddr_in servaddr, socklen_t len){
	//Invio al server cosa voglio fare
	err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
	/*attesa rispsta del server*//*restituira dentro buffer la lista degliu elementi disponibili*/
	bzero(buffer,DIMENSIONE_MESSAGGI);
	recvfrom(socketone,buffer,sizeof(buffer),0,(SA *) &servaddr, &len);
	printf("--------------------------------\nLista dei file disponibili nel server:\n%s\n--------------------------------", buffer);
}
	
	
/*Questa funzioen serve per mandare dei messaggi con un algoritmo scelto (GO-BACK-N) 
La funzione manda preliminarmente tutti i pacchetti dentro la window dimensione(finestra di trasmissione)
per poi mettersi in attesa dei loro riscontri, facendo scorrere la finestra ogni qual volta arriva un ack.
La funzione associa il timer al primo pacchetto della finestra, ed ogni volta che esso viene riscontrato viene fatto
partire il timer associato al nuovo leader della finestra(il primo)	*/
int send_packet_GO_BACK_N(struct inside_the_package *file_struct, int seq, int offset){//offset ha il valore della WINDOWS dimensione per i pacchetti "multipli di windows dimensione" e offset per i pack "non multipli"
	int i;
	int j;
	si = 0; //indice di partenza per riscontrar ei pack
	int timer=1; //utile per far partire il timer solo del primo pacchetto
	printf("\n------------------------------------------------------------------------\n");
	/*Ciclo for che invia offset pacchetti alla volta*/
	for(i = 0; i < offset; i++){	
		//imposto l'ack del pacchetto che sto inviando come 0, lo metterò a 1 una volta ricevuto l'ack dal client
		/*seq(inzialmente uguale a 0), indica il numero del pack*/
		file_struct[seq+i].ack = 0;
		/*Mando il primo pack*/
		if(sendto(socketone, file_struct[seq+i].buf, DIMENSIONE_MESSAGGI, 0, (struct sockaddr *) &servaddr, len) < 0) { 
			fprintf(stderr, "Errore nell'invio del pacchetto numero: %d.\n", seq);
			exit(EXIT_FAILURE);
		}
		printf("Pacchetto [%d] inviato\n",seq+i);
		if(timer==1){
			/*Qui dovrebbe partire il timer associato al pack seq*/
			setTimeout(SEND_FILE_TIMEOUT,seq+i);//timeout del primo pack
		}
		timer=0;
		bzero(buffer, DIMENSIONE_MESSAGGI);
		
	}
	printf("\n------------------------------------------------------------------------\n");
	/*Entro nella fase di attesa dei riscontri dei pacchetti*/
	for(j = si; j < offset; j++){
		printf("Attendo ACK [%d]\n",seq+si);
		bzero(buffer, DIMENSIONE_MESSAGGI);
		int err = recvfrom(socketone,buffer, DIMENSIONE_MESSAGGI, 0, (SA *) &servaddr, &len);//Vado in attesa del riscontro da parte del server
		errno=0;
		/*Caso perdo il pacchetto*/
		if (err < 0){
			if(errno == EAGAIN){
				/*
				Nel caso viene perso un pacchetto mi sposto di nuovo dentro il ciclo for, permettendo di nuovo l'invio dei pacchetti non riscontrati
				*/
				printf("Il pacchetto è andato perso o danneggiato, ack: %d non ricevuto\n\n\n------------------------------------------------------------------------\n",seq);
				i=id;//Utile per impostare l'indice nel ciclo for, indica l'id del pacchetto di cui mi aspetto un riscontro
				si=id;//Utile per impostare l'attesa degli ack dei pacchetti ritrasmessi
				seq=seq-i;
				bzero(buffer, DIMENSIONE_MESSAGGI);
				}
		
			else{
				printf("Timer scaduto...\n");
				
			}
		}
		/*Caso prendo il pacchetto*/
		else{
			int check = atoi(buffer);//Prendo l'id del pacchetto riscontrato dal server
			bzero(buffer, DIMENSIONE_MESSAGGI);
			if(check >= 0){
				file_struct[check].ack = 1; //Imposto l'ack del pacchetto all'interno della struttura uguale a 1, indicando che tale pack è stato riscontrato
			}
			if(check!=num){//caso ack diverso da quello che mi aspettavo
				printf("Ho ricevuto un ack di un pacchetto gia riscontrato o un ack danneggiato, resto in attesa\n\n");
				id++;
			}
			else{//caso ack uguale a quello aspettato
				printf("Ho ricevuto l'ack del pacchetto [%d]\n\n",check);
				setTimeout(SEND_FILE_TIMEOUT,seq+si);//timeout per i pack successivi al primo
				
				/*
				Questo controllo su seq serve per far scorrere l'id del pacchetto ricercato in mainera ottimale
				In particolare andrà ad indicare quale sarà il prossimo pacchetto da riscontrare:
				- Se seq è uguale a 0 indica che ho riscontrato il pacchetto 0-esimo e che mi aspetto il pacchetto 1-esimo
				- Nel resto dei casi si passa dal pacchetto i-esimo a quello (i+1)-esimo
				*/
				if(seq==0){
					seq=seq+check+1; //Avviene solo nella trasmissione del primo pacchetto, server per indicare quale sarà il prossimo pack da riscontrare
				}
				else{
					seq=check+1; //Quello successivo a quello riscontrato correttamente, cioè quello da inviare di nuovo in caso di perdita
					}		
				/*Faccio ripartire il timer al pack seq*/
				num++;
				id++;
			}
		}
	}
	return seq;
}


/*Questa funzione serve per settare un tempo alla richiesta della socket*/
void setTimeout(double time,int id) {
	struct timers timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = time;
    setsockopt(socketone, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}
