#include<string.h>
#include<stdlib.h>
#include<stdio.h>
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
#include <signal.h>




/*Valori definiti preliminarmente*/
#define PORT 8090 //Porta di default per l'inizio delle conversazioni client-server
#define DIMENSIONE_MESSAGGI 1064 //Dimensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr //Struttura della socket
#define CODICE 25463 //Codice di utility per gestire la chiusura del server
#define DIMENSIONE_PACCHETTO 1024 // dimensione del payload nel pacchetto UDP affidabile
#define DIMENSIONE_FINESTRA 3 // dimensione della finestra di spedizione
#define TIMEOUT 100000 // timeout di invio
#define L_PROB 15 // probabilità di perdita
#define USCITA 1998


/*strutture*/
struct inside_the_package{
	int numero_ordine;
	char buf[DIMENSIONE_MESSAGGI];
	int ack;
};

/*Dichiarazioni funzioni*/
void List_of_files(int, struct sockaddr_in, socklen_t);
int Socket_creation(int);
void setTimeout(double,int);
void *esci();
void Reception_data();
void UDP_GO_BACK_N_Recive();
int sendACK(int ,int );
int upload();
int downlaod();
int num_random();
int UDP_GO_BACK_N_Send(struct inside_the_package *, int , int );
char * parsed(int , char []);





/*Variabili globali*/
int si; //utile per il riscontro dei pack
int num=0; //per vedere i pack fuori ordine
int last_recive=0; //indica l'ultimo pacchetto ricevuto correttamente
struct timeval t;//Struttura per calcolare il tempo trascorso
struct sockaddr_in servaddr;// struct di supporto della socket
socklen_t len;//Lunghezza della struct della socket
char file_name[128];	//Buffer per salvare il nome del file
int num_pacchetti;	// numero di pacchetti da inviare/ricevere
char buffer[DIMENSIONE_MESSAGGI]; 	//Buffer unico per le comunicazioni
char *lista_dei_files; 	// buffer per il contenuto della lista di file
int window_base = 0; 	// parametro di posizionamento attuale nella spedizione
int socketone;	//File descriptor della socket
int err;//Variabile per controllo di errore
int dimensione; 	// dimensione del file da trasferire
char *file_da_ricevere;
char **buff_file;
int id=0;
int c_error; //variabile per la gestione degli errori
int exist=0; //per vedere se un file è gia esistente
struct timeval  tv1, tv2, tv3, tv4; //per il calcolo del tempo di esecuzione


int main() {
	signal(SIGINT,(void*)esci);
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
	if(port_number==CODICE){
		printf("Server pieno, riprova più tardi!\n");
		exit(0);
	}
	printf("\n[CONNESSO ALLA PORTA %d]\n",port_number);
	//Creo la socket sulla porta passata dal server
	socketone = Socket_creation(port_number);
	//Ciclo infinito di richieste
	while(1){
		signal(SIGINT,(void*)esci);
		if(atoi(buffer) == CODICE){
				perror("ATTENZIONE! Il server non è più in funzione.");
				return 1;
		}
		//Faccio una pulizia preliminare del buffer
		bzero(buffer, DIMENSIONE_MESSAGGI);
		//Mostro a schermo le possibili scelte
		printf("Inserisci un comando tra: \n1) exit\n2) list\n3) download \n4) upload\n");
		//Inserisco nel buffe rla linea di richiesta del client
		printf("\nComando:");
		fgets(buffer, DIMENSIONE_MESSAGGI, stdin);
		//Verifico se il client vuole uscire o meno dal ciclo
		
		if(((strncmp("1", buffer, strlen("1"))) == 0)&& (strlen(buffer)<5)){//Caso di uscita
		
			printf("Il client sta chiudendo la connessione...\n");
			//invio il messaggio al server per notificargli la chiusura del client
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
			List_of_files(socketone,servaddr,len);
			
		}
		
		//CASO UPLOAD
		else if ((strncmp("4", buffer, strlen("4"))) == 0) {
			
			
			upload();
			
		}
		//CASO DOWNLOAD
		else if((strncmp("3", buffer, strlen("3"))) == 0) {

			downlaod();

			
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
Questa funzione serve a preparare il server alla ricezione dei dati allocando memoria sufficente per contenerli
In particolare riceve informaizoni come il nome, la dimensione
ed in base ad esse si calcola il numero di pacchetti da ricevere
In base al nome ricevuto essa crea un file dello stesso nome all'interno della repository
e alla fine della ricezione copia l'intero contenuto dei pack ricevuti su quel file
In fine aggiorna la lista dei file disponibili sul server
*/
void Reception_data(){
	printf("Avviata procedura di ricezione del file\n");
	bzero(buffer, DIMENSIONE_MESSAGGI);
	/*Attendo la lunghezza del file*/
	recvfrom(socketone, buffer, DIMENSIONE_MESSAGGI, 0, (SA *) &servaddr, &len);
	int dim_file=atoi(buffer);
	printf("Ho ricevuto un file di lunghezza :%d\n",dim_file);
	/*Alloco la memoria per contenerlo*/
	buff_file=malloc(dim_file);
	/*
	Mediante la chiamata ceil:
	La funzione restituisce il valore integrale più piccolo non inferiore a x .
	*/
	num_pacchetti = (ceil((dim_file/DIMENSIONE_PACCHETTO)))+1;
	printf("Numero pacchetti da ricevere: %d.\n", num_pacchetti);
	/*
	Utilizzo questa tecnica per capire quanti pacchetti dovrò ricevere
	Alla fine della procedura avro a disposizione sia il nome del file e la sua lunghezza
	*/
   	for(int i = 0; i < num_pacchetti; i++){
    	buff_file[i] = mmap(NULL, DIMENSIONE_MESSAGGI, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
    	if(buff_file[i] == NULL){
			herror("c_errorore nella mmap del buff_file della receive_name_and_len_file del server.");
		}
    }
	
	int fd = open(file_name, O_CREAT|O_RDWR, 0666);
	printf("Inizio a ricevere i pacchetti con GO-BACK-N\n\n\n");
	JUMP:
	UDP_GO_BACK_N_Recive();	
	printf("\n\n[RICEZIONE TERMINATA CORRETTAMENTE]-----------------------------------------\n");
	printf("Sto creando il file %s\n", file_name);	
	/*Funzione che riscrive l'intera struttura nel file*/
	for(int i = 0; i < num_pacchetti; i++){
		int ret = write(fd, buff_file[i], strlen(buff_file[i]));
		if(ret == -1){
			herror("c_errorore nella write della write_data_packet_on_local_file del server.");
		}
	}
	
	
	printf("File scritto correttamente.\n");
	/*Aggiorno la lista dei file*/
	printf("Aggiorno file_list...\n");
	FILE *f;
	f=fopen("lista_c.txt","a");
	printf("FILE NAME %s\n",file_name);
	fprintf(f, "\n%s", file_name); 
	printf("File aggiornato correttamente.\n[OPERAZIONE COMPLETATA CON SUCCESSO]\n");
	close(fd);
	fclose(f);
	num=0;
	return;
}

/*
Questa funzione veien utilizzata per creare socket
Viene creata una socket e la struct di supporto
Viene ritornata la socket
*/
int Socket_creation(int c_port){
	// creazione della socket
	int c_socketone = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// salvo in len lunghezza della struct della socket
	len = sizeof(servaddr);
	// controllo d'errore nella creazione della socket
	if(socketone == -1){
		perror("ATTENZIONE! Creazione della socket fallita...");
	}
	// pulisco la memoria allocata per la struttura
	bzero(&servaddr, sizeof(servaddr));
	// setto i parametri della struttura
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(c_port);
	// binding della socket con controllo d'errore
	return c_socketone;
}


/*
Questa funzione viene utilizzata per richiedere una lista di file al server
Vado in attesa di ricevere questa lista dal client mediante la socket
Stampo la lista su stdout
*/
void List_of_files(int socketone, struct sockaddr_in servaddr, socklen_t len){
	//Invio al server cosa voglio fare
	err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
	if (err < 0){
				perror("Errore nella sendto nella sezione del servizio di list del client.");
			}
	/*attesa rispsta del server*//*restituira dentro buffer la lista degliu elementi disponibili*/
	bzero(buffer,DIMENSIONE_MESSAGGI);
	recvfrom(socketone,buffer,sizeof(buffer),0,(SA *) &servaddr, &len);
	if(atoi(buffer) == CODICE){
				perror("ATTENZIONE! Il server non è più in funzione.");
				return;
	}
	printf("[LISTA DEI FILE NEL SERVER]---------------------------------------------------\n%s\n-----------------------------------------------------------------------------------------------\n", buffer);
}
	
	
/*Questa funzioen serve per mandare dei messaggi con un algoritmo scelto (GO-BACK-N) 
La funzione manda preliminarmente tutti i pacchetti dentro la window dimensione(finestra di trasmissione)
per poi mettersi in attesa dei loro riscontri, facendo scorrere la finestra ogni qual volta arriva un ack.
La funzione associa il timer al primo pacchetto della finestra, ed ogni volta che esso viene riscontrato viene fatto
partire il timer associato al nuovo leader della finestra(il primo)	*/
int UDP_GO_BACK_N_Send(struct inside_the_package *file_struct, int seq, int not_mult){//not_mult ha il valore della WINDOWS dimensione per i pacchetti "multipli di windows dimensione" e not_mult per i pack "non multipli"
	
	int i;
	int j;
	si = 0; //indice di partenza per riscontrar ei pack
	int timer=1; //utile per far partire il timer solo del primo pacchetto
	printf("\n--------------------------------------------------------------------------------------------------------\n");
	/*Ciclo for che invia not_mult pacchetti alla volta*/
	for(i = 0; i < not_mult; i++){	
		if(seq+i>=num_pacchetti){
			goto WAIT;
			//si=i-1;
		}
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
			setTimeout(TIMEOUT,seq+i);//timeout del primo pack
		}
		timer=0;
		bzero(buffer, DIMENSIONE_MESSAGGI);
		
	}
	printf("\n--------------------------------------------------------------------------------------------------------\n");
	int seq2=seq;
	/*Entro nella fase di attesa dei riscontri dei pacchetti*/
	for(j = si; j < not_mult; j++){
		WAIT:
		if(seq2+j>=num_pacchetti){
			seq++;
			goto FINE;
		}
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
				printf("Ho ricevuto un ack del pacchetto %d, ma mi aspettavo %d\n\n",check,num);
				id++;
			}
			else{//caso ack uguale a quello aspettato
				printf("Ho ricevuto l'ack del pacchetto [%d]\n\n",check);
				setTimeout(TIMEOUT,seq+si);//timeout per i pack successivi al primo
				
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
	FINE:
	bzero(buffer, DIMENSIONE_MESSAGGI);
	return seq;
}


/*Questa funzione serve per settare un tempo alla richiesta della socket*/
void setTimeout(double time,int id) {
	struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = time;
    setsockopt(socketone, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void *esci(){
	printf("\n");
	bzero(buffer, DIMENSIONE_MESSAGGI);
	sprintf(buffer, "%d", USCITA);
	err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
	// pulisco il buffer
	bzero(buffer, DIMENSIONE_MESSAGGI);
	if (err < 0){
		perror("Errore nell'invio del messaggio di chiusura da parte del client\n");
	}
	// chiudo la socket
	close(socketone);
	bzero(buffer, DIMENSIONE_MESSAGGI);
	exit(1);
}


void UDP_GO_BACK_N_Recive(){
	last_recive=0;
	num=0;
	/*
	Il server conoscendo la windows size, la dimensione per ogni pack, e la grandezza del file, può calcolarsi
	quante ondate di pacchetti "normali" e "diversi" arriveranno dal client
	*/
	int seq,timer; //dorvò scegliere opportunamente il timer di scadenza
	int contatore = 0;
	struct inside_the_package pacchett[num_pacchetti];//Creo la struttura per contenere i pacchetti
	int not_mult = num_pacchetti%DIMENSIONE_FINESTRA; //indica quante ondate di pacchetti devo ricevere 
	int w_size=DIMENSIONE_FINESTRA;//w_size prende la dimensione dell DIMENSIONE_FINESTRA per poi riadattarla in caso di pack "diversi"
	/*Vado nel ciclo finche non termino i pacchetti*/
	gettimeofday(&tv3, NULL);//Parte timer per valutare le prestazioni
	while(num < num_pacchetti){
		/*Inizio a ricevere pacchetti*/
		/*
		Entriamo in questo ciclo solo nel caso in cui rimangono al piu not_mult pacchetti, 
		di conseguenza la dimensione della nostra finestra diventa not_mult
		 */
		if(num_pacchetti-contatore <= not_mult + 1 && not_mult!=0){
			if(DIMENSIONE_FINESTRA%2){
				w_size = not_mult;
			}
			else
			{
				w_size = not_mult +1;
			}
		}
		/*
		1)w_size ha dimensione DIMENSIONE_FINESTRA nel caso di pach "normali" 
		2)w_size ha dimensione not_mult nel caso di pach "diversi" 
		*/
		for(int i = 0; i <w_size; i++){
			CICLO:
			bzero(buffer, DIMENSIONE_MESSAGGI);
			printf(" ");
			char pacchetto_ric[DIMENSIONE_MESSAGGI];
			char *pacchetto_contenuto;
			pacchetto_contenuto = malloc(DIMENSIONE_PACCHETTO);
			
			/*attesa di un pacchetto*/
			c_error = recvfrom(socketone, pacchetto_ric, DIMENSIONE_MESSAGGI, 0, (SA *) &servaddr, &len);
			if (c_error < 0){
					herror("c_errorore nella recvfrom della recive_UDP_rel_file nel client");
			}
			
			// buff riceve il numero di sequenza messo nel header del pacchetto
			char *buff;
			const char s[2] = " ";
			buff = strtok(pacchetto_ric, s);//divido il pacchetto in piu stringhe divise da s e lo metto in buf tutto segmentato
			int k = atoi(buff); //i prende il numero di sequenza nell'headewr del pacchetto
			seq=k;//seq prende il numero di sequenza nell'headewr del pacchetto
			if(seq!=num){
				printf("Pacchetto %d ricevuto fuori ordine, ora lo scarto e rimando l'ack di %d\n",seq,(last_recive-1));
				if(sendACK((last_recive-1),DIMENSIONE_FINESTRA)){//invio l'ack del pacchetto antecedente a quello che mi sarei aspettato
					goto CICLO;
				}
				else{
					goto NOOK;
				}
			
			}
			
			
			if(seq >= contatore){
				contatore = seq + 1;
			}
			printf("--------------------------------------[HO RICEVUTO IL PACCHETTO %d]------------------------------------\n",seq);
			 //indico che è lui il nuovo pacchetto ricevuto
			pacchetto_contenuto = parsed(seq,pacchetto_ric);
			printf("\t\t\t\t\t[CONTENUTO PACK %d-ESIMO]\n%s\n-----------------------------------------------------------------------------------------------------------\n",seq,pacchetto_contenuto);
			
			/*Ora devo mandare gli ack */
			if(sendACK(seq,DIMENSIONE_FINESTRA)){
			last_recive++;
				// copia del contenuto del pacchetto nella struttura ausiliaria
				if(strcpy(pacchett[seq].buf, pacchetto_contenuto) == NULL){
					exit(-1);
				}
				if (strcpy(buff_file[seq], pacchett[seq].buf) == NULL){
					exit(-1);
				}
				printf("Pacchetto riscontrato numero di seq: %d.\n", seq);	
				//incremento il contatore che mi identifica se il pack è in ordine		
				num=seq+1;
			}
			else{
				NOOK:
				printf("Pacchetto NON riscontrato numero di seq: %d.\n", seq);
				goto CICLO;
				
			}
			free(pacchetto_contenuto);
			FINE:
			printf(" ");	
			}		
	}
	gettimeofday(&tv4, NULL);//fermo il timer
	printf("Downlaod total time = %f seconds\n",(double) (tv4.tv_usec - tv3.tv_usec) / 1000000 +(double) (tv4.tv_sec - tv3.tv_sec));
	return;
}


/*Questa funzione permette di estrapolare il contenuto del pacchetto prendendo solo le informazioni che ci servono -> (testo)*/
char * parsed(int seq, char pacchetto_ric[]){
	/*La funzione restituisce la sottostringa del pacchetto -> PASSA MALE IL CONTENUTO
			contentente il messaggio vero e proprio*/
			char *c_index;
			c_index = malloc(DIMENSIONE_PACCHETTO+8);		
			sprintf(c_index, "%d", seq);
			int st = strlen(c_index) + 1;
			char *start = &pacchetto_ric[st];
			char *end = &pacchetto_ric[DIMENSIONE_MESSAGGI];
			char *substr = (char *)calloc(1, end - start + 1);
			memcpy(substr, start, end - start);
			free(c_index);
			return substr;
}

/*Questa funzione viene utilizzata per mandare gli ACK*/
int sendACK(int seq,int WINDOW_SIZE){
	int loss_prob;
	bzero(buffer, DIMENSIONE_MESSAGGI);
	sprintf(buffer, "%d", seq);
	if(seq > num_pacchetti-WINDOW_SIZE-1)
	{
		loss_prob = 0;
	}
	else
	{
		loss_prob = L_PROB;
	}
	int random = num_random(); 
	if(random < (100 - loss_prob)) {
		c_error = sendto(socketone, buffer, DIMENSIONE_MESSAGGI, 0, (SA *) &servaddr, len);
		bzero(buffer, DIMENSIONE_MESSAGGI);
		if(c_error < 0){
			herror("c_errorore nella sendto della sendACK del server.");
		}
		return 1;
	}
	else
	{
		bzero(buffer, DIMENSIONE_MESSAGGI);
		return 0;
	}
}

/*Questa funzione serve per creare numeri random da 0 a 100*/
int num_random(){
	return rand()%100;
}

/*Funzione utilizzata per effettuare l'upload di un file*/
int upload(){
	//Invio al server l'azione che voglio fare
			id=0;
			err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			if (err < 0){
				perror("Errore nella sendto nella sezione del servizio di upload del client.");
			}
			printf("[FASE DI UPLOAD]\n");
			/*Attendo che il Server mi dia il permesso per proseguire*/
			bzero(buffer, DIMENSIONE_MESSAGGI);
			
			/*Attendo permesso dal server*/
			err = recvfrom(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, &len);
			if (err < 0){
				perror("Errore nella recvfrom nella sezione del servizio di upload del client.");
			}
			if(atoi(buffer) == CODICE){
				perror("ATTENZIONE! Il server non è più in funzione.");
				return 1;
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
			if(file<0){
				printf("Errore apertura lista dei file");
			}
			/*
			Calcolo la lunghezza complessiva del file facendo scorrere la testina dall'inzio alla fine
			lseek ritornerà l'intera grandezza del file
			*/
			lseek(file,0,0);
			dimensione = lseek(file,0,SEEK_END);//Scorro la testina dall'inzio alla fine
			lista_dei_files = malloc(dimensione);//Alloco tanta memoria per contenerlo
			lseek(file,0,0);//riposiziono la testina all'inzio del file(meotodo di accesso diretto)
			
			/*Lettura dei della lista dei file caricabili sul server*/
			read(file,lista_dei_files, dimensione);
			printf("[LISTA DEI FILE NEL CLIENT]-------------------------------------\n%s\n-------------------------------------------------------------------\n",lista_dei_files);
			
			/*Scelta del file*/
			bzero(buffer, DIMENSIONE_MESSAGGI);
			SCELTA:
			printf("Scegli il file inserendo il suo nome per intero(compreso di .txt): ");
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
			
			printf("[INFORMAZIONI PRELIMINARI]------------------------------------------------\n");
			/*Calcolo quanti pacchetti devo inviare al server*/
			dim = lseek(file_inv, 0, SEEK_END);
			num_pacchetti = (ceil((dim/DIMENSIONE_PACCHETTO)))+1;
			printf("Numero di pacchetti da caricare: %d.\n", num_pacchetti);
			printf("File di lunghezza %d\n",dim);
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
			for(int i = 0; i < num_pacchetti; i++){
				bzero(file_struct[i].buf, DIMENSIONE_MESSAGGI);
			}
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
			printf("[CONTENUTO FILE_STRUCT]------------------------------------------------\n");
			/*Stampo l'inter struttura*/
			for (int i = 0; i < num_pacchetti; i++){
				printf("\nFILE_STRUCT[%d].BUF contiene :\n%s\n--------------------------------------------------------------\n",i,file_struct[i].buf);
			}
			
			/*Da qui in poi ho tutti i pacchetti salvati nella struttura*/
			printf("[PACCHETTI CARICATI NELLA STRUTTURA]\n\n");
			printf("[FASE DI SCAMBIO]---------------------------------------------------\n");
			/*Fase di invio dei pacchetti*/
			
			/*Vedo quanti pacchetti non sono multipli della windows dimensione*/
			int not_mult = (num_pacchetti)%DIMENSIONE_FINESTRA;
			/*Caso in cui il numero dei pacchetti da inviare in maniera diversa perche non sono un multipli della WINDOWS_SIZE*/
			//if(not_mult > 0){//Numoero di pacchetti da inviare "multipli di WINDOWS_SIZE"*/
			gettimeofday(&tv1, NULL);
			while(seq<num_pacchetti - not_mult){//fino a quando non sono arrivato al primo pack "NON multiplo di WINDOWS_SIZE"
				seq = UDP_GO_BACK_N_Send(file_struct, seq, DIMENSIONE_FINESTRA);//mando la struttura contenente i pacchetti, la sequenza, e la dimensione della finestra			
				bzero(buffer, DIMENSIONE_MESSAGGI);
			}
			/*Una volta inviati i pacchetti "multipli di WINDOWS_SIZE" invio not_mult pacchetti "NON multipli di WINDOWS_SIZE"*/
			if(seq<num_pacchetti){		/*Nel caso ne rimangano alcuni, li invio*/
				seq = UDP_GO_BACK_N_Send(file_struct, seq, not_mult);//mando la struttura contenente i pacchetti, la sequenza, e il numero di pacchetti rimanenti
				bzero(buffer, DIMENSIONE_MESSAGGI);
			}
			gettimeofday(&tv2, NULL);
			printf ("Upload total time = %f seconds\n",(double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +(double) (tv2.tv_sec - tv1.tv_sec));
			printf("[FINE FASE]----------------------------------------------------\n");	
			bzero(buffer, DIMENSIONE_MESSAGGI);
			FINISH:
			/*Reset delle informaizoni*/
			si=0;
			seq=0;
			num=0;
			num_pacchetti=0;
			id=0;
			bzero(buffer, DIMENSIONE_MESSAGGI);
}
/*Funzione utilizzata per effettuare il download di un file*/
int downlaod(){
	//Invio//Invio al server cosa voglio fare
			
			err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			if (err < 0){
				perror("Errore nella sendto nella sezione del servizio di upload del client.");
			}
			/*attesa rispsta del server*/
			if(atoi(buffer) == CODICE){
				perror("ATTENZIONE! Il server non è più in funzione.");
				return 1;
			}
			
			printf("[DOWNLOAD]----------------------------------------------------------\n");
			// ricevo la lista di file che posso scaricare
			err = recvfrom(socketone, buffer, DIMENSIONE_MESSAGGI, 0, (SA *) &servaddr, &len);
			char *listfile=buffer;
			if (err < 0){
				perror("Errore nella recvfrom nella sezione del servizio di download del client.");
			}
			printf("Download accettato dal server.\n");
			
			
			// Riucevo la lista dei file dal server
			printf("[LISTA DEI FILE NEL SERVER]---------------------------------------------------\n%s\n--------------------------------------------------------------------------------\n", buffer);
			bzero(buffer, DIMENSIONE_MESSAGGI);
			REQUEST:
			printf("Inserisci il nome del file da scaricare: ");
			
			
			// riempio il buffer con la stringa inserita in stdin
			fgets(buffer, DIMENSIONE_MESSAGGI, stdin);
			int leng = strlen(buffer);
			if(buffer[leng-1] == '\n'){
				buffer[leng-1] = '\0';
			}
			strcpy(file_name,buffer);


			// invio la richiesta con il nome del file da scaricare preso 
			// dalla lista ricevuta dal server
			err = sendto(socketone, buffer, sizeof(buffer), 0, (SA *) &servaddr, len);
			if (err < 0){
				perror("Errore nella sendto 2 nella sezione del servizio di download del client.");
			}
			
			//ricevo un messaggio dal server per capire se il nome inviato è giusto
			bzero(buffer,DIMENSIONE_MESSAGGI);
			err = recvfrom(socketone, buffer, DIMENSIONE_MESSAGGI, 0, (SA *) &servaddr, &len);
			if (err < 0){
				perror("Errore nella recvfrom nella sezione del servizio di download del client.");
			}
			if(strcmp(buffer,"not_exist")==0){
				printf("Il file non esiste sul server\n");
				bzero(buffer, DIMENSIONE_MESSAGGI);
				goto REQUEST;
			}
			// pulisco il buffer
			bzero(buffer, DIMENSIONE_MESSAGGI);
			Reception_data();
			bzero(buffer, DIMENSIONE_MESSAGGI);
}
