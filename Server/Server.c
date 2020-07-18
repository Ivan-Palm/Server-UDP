#include<stdio.h>
#include<sys/types.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/time.h>
#include<signal.h>
#include<math.h>
#include<sys/mman.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<errno.h>


/*Valori definiti preliminarmente*/
#define MAX_DIM_MESSAGE 1064 //Diensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr //Struttura della socket
#define CONNESSIONI 5


/*strutture*/
struct pacchetto{
	int numero_ordine;//indica la posizione
	char buf[MAX_DIM_MESSAGE];//dati
	int ack;//indica se è un riscontro
};


/*Dichiarazioni funzioni*/

void f_esci(int , int , pid_t);
void f_lista(int, struct sockaddr_in, socklen_t);
void *exit_t();
void *esci();
void child_exit();
void f_upload(int , struct sockaddr_in , socklen_t );
void f_download(int , struct sockaddr_in , socklen_t );
void reception_data();
int SEND_ACK(int ,int );
int creazione_socket(int);
int num_random();
int chiudi_socket(int);
void show_port();
int decrement_client(int*);
void *exit_p();
void setTimeout(double,int);
int GO_BACK_N_Send(struct pacchetto *, int , int );
char * parsed(int , char []);




/*Variabili globali*/
int WINDOW_SIZE=3;
int received=0; //indica l'ultimo pacchetto ricevuto correttamente
int num_pack; 	// numero di pacchetti da inviare
char **buff_file;
char pathname[1024];
int number=0;//contatore utile per verificare l'ordine dei pacchetti
int **numeri_di_porta;
int *utenti_connessi;
int num_port[CONNESSIONI];
void *exit_t();
char *buff_file_list; //send_buff per il contenuto della lista di file
char send_buff[MAX_DIM_MESSAGE]; //send_buff per comunicare con i client
int s_socketone;//File descriptor della socket per i child
pid_t parent_pid; //PID del parent nel main
int size; //Dimensione del file da trasferire
struct sockaddr_in servaddr;//Struct di supporto della socket
socklen_t len;//Lunghezza della struct della socket
int port_number = 0; //Variabile di utility per il calcolo delle porte successive da dare al client
int port_client; //Porta che diamo al client per le successive trasmissioni multiprocesso
int socketone;	//File descriptor di socket
int c_error; 	// intero per il controllo della gestione d'c_errorore
int parent; //pid del processo padre
int num_pacchetti;
int id=0;
int exist=0; //per vedere se un file è gia esistente
int Porta_chiusura = 8089;
int Default_port = 8090;
int Dimension = 1024;
int Code = 25463;
int Dimensione_pacchetto = 1024;
int Perc_perdita = 15;
int Dimensione_finestra=3;
int Timer = 10000;
int si=0;


int decrement_client(int *utenti_connessi){
	*utenti_connessi=*utenti_connessi-1;
	return *utenti_connessi;
}


/*
Questa funzione serve per mostrare al server quali porte ha libere, e di conseguenza quanti utenti possono
ancora accedervi senza creare una sovraffolazione
*/
void show_port(){
	/*Creo un array di interi accessibile da piu processi, col fine di capire quale porte sono libere o meno*/
	numeri_di_porta=malloc((CONNESSIONI*sizeof(int*)));
	if(numeri_di_porta==NULL){
		printf("Problema creazione array contenente i numeri di porta\n");
		exit(-1);
	}
	for(int k=0;k<CONNESSIONI;k++){
		numeri_di_porta[k]=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_SHARED,0,0);
		if (numeri_di_porta[k] == NULL){
			printf("mmap c_erroror\n");
			exit(-1);
		}
	}
	for(int i=0;i<CONNESSIONI;i++){
		*numeri_di_porta[i]=i+1+Default_port;
	}
	printf("\n-------------------------Porte disponibili:-------------------------\n");
	for(int l=0;l<CONNESSIONI;l++){
			printf("\t\t\t\t[%d]\n",*numeri_di_porta[l]);
	}
	
}

int increse_client(int *utenti_connessi){
	*utenti_connessi = *utenti_connessi + 1;
	return *utenti_connessi;
}


void define_num_client(){
	utenti_connessi=malloc((CONNESSIONI*sizeof(int)));
		if(utenti_connessi==NULL){
			printf("Problema creazione dell'array di interi indicante gli utenti connessi\n");
			exit(-1);
		}
		utenti_connessi = mmap(NULL,105,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_SHARED,0,0);
		*utenti_connessi=0;
}

int main(){
	signal(SIGINT,(void*)exit_p);//esco nel caso il child termina
	show_port();
	define_num_client();
	/*Salvo il pid del processo padre in una variabile globale*/
	parent_pid = getpid();
	
	//creo la socket di comunicazione per i child
	s_socketone = creazione_socket(Default_port);
	
	
	while(1){
		bzero(send_buff, MAX_DIM_MESSAGE);
		//attendo un client
		if(recvfrom(s_socketone, send_buff, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, &len) < 0){
			herror("c_errorore nella recvfrom nel primo while del server.");
		}

		bzero(send_buff, MAX_DIM_MESSAGE);
	
		//aumento contatore che segnala i client attivi
		*utenti_connessi=increse_client(utenti_connessi);
		
		//verifico se ho superato il range di client ammissibili in tal caso li diminusco
		if(*utenti_connessi>CONNESSIONI){
			printf("Numero massimo di client raggiunto!\n");
			esci();//FORSE NON CI VA
		}
		else{
			//aggiorno il numero di porta sulla quale fare connettere i client
			port_number = ((port_number )%(CONNESSIONI))+1;
			//aggiorno numero di porta da passare al client
			port_client = Default_port + port_number;//il primo avra 8091 il secondo 8092 e cosi via...
			int k=0;
			
			/*Procedura per la ricerca del numero di porta non utilizzato
			Questa procedura permette di marcare con uno zero i numeri di porta utilizzati
			Il child ed il parent utilizzeranno delle pagine di memoria condivisa per capire se una porta è libera
			o no. Nel caso fosse libera viene impostata a 0. (Simile allo scheduler dei processi I/O NRU)
			*/
			for(int j=0;j<CONNESSIONI;j++){
				if(*numeri_di_porta[j]!=0){
					port_client = *numeri_di_porta[j];
					*numeri_di_porta[j]=0;
					break;
				}
				else{
					k++;
				}
			}
			if(k==CONNESSIONI)
			{
				printf("Non ho porte libere\n");
				//invio sengale al client
			}
			printf("\n-------------------------Porte disponibili:-------------------------\n");
			for(int l=0;l<CONNESSIONI;l++){
				printf("\t\t\t\t[%d]\n",*numeri_di_porta[l]);
			}
			printf("\n------------------------------NUOVO UTENTE CONNESSO!Client Default_port %d------------------------------\n",port_client);
			bzero(send_buff, MAX_DIM_MESSAGE);
			//scrivo il valore aggionrato nel send_buff di comunicazione
			sprintf(send_buff,"%d",port_client);
			//comunico al client su quale porta si sta connettendo
			if(sendto(s_socketone, send_buff, MAX_DIM_MESSAGE,0, (struct sockaddr *) &servaddr, len) < 0){
				herror("c_errorore nella sendto 2 del primo while del main del server.");
			}
			bzero(send_buff, MAX_DIM_MESSAGE);
			printf("UTENTI CONNESSI %d\n",*utenti_connessi);
			/*
			Creo un child per ogni connessione, esso la gestirà mentre il padre rimarrà in ascolto di nuove eventuali connessioni
			Questa fase è molto importante in quanto ogni volta che viene richiesta una connessione viene creato un child che 
			avrà il compito di gestirla; tale tecnica viene utilizzata per aumentare il livello di efficenza del server, in quanto ci saranno
			piu "lavoratori" attivi in conbufforanea.
			*/
			pid_t pid = fork();
			if(pid == 0){
				/*
				Entro nella gestione della singola connessione verso un client
				Creo una nuova socket per questa connessione e chiudo la socket di comunicazione del padre
				*/
				signal(SIGINT,(void*)exit_t);
				socketone = creazione_socket(port_client);//apro
				chiudi_socket(s_socketone);/*Chiudo la socket non utilizzata dal child ereditata dal parent*/			
				/*creo i segnali per la gestione del child*/
				/*Entro nel ciclo di ascolto infinito*/
				while(1){
					REQUEST:
					bzero(send_buff, MAX_DIM_MESSAGE);//Pulisco il send_buff
					/*Vado in attesa di un messaggio*/
					memset(send_buff,0,MAX_DIM_MESSAGE);
					if(recvfrom(socketone, send_buff, sizeof(send_buff), 0, (struct sockaddr *) &servaddr, &len) < 0){
						if (errno==EAGAIN)
						{
							goto REQUEST;
						}
						else{
							herror("c_errorore nella recvfrom del secondo while del main del server.");
						}
					}
					/*Gestisco la richiesta del client*/
					/*Caso exit*/
					if(strncmp("1", send_buff, strlen("1")) == 0){
						printf("Client Default_port %d -> Richiesto exit\n",port_client);
						printf("[FASE EXIT]\n");
						f_esci(port_client,socketone,parent_pid);
						while(1){
							sleep(500);
						}
						bzero(send_buff, MAX_DIM_MESSAGE);
					}
					
					/*Caso list*/
					else if(strncmp("2", send_buff, strlen("2")) == 0){
						printf("Client Default_port %d -> Richiesto list\n",port_client);
						printf("[FASE LIST]\n");
						f_lista(socketone,servaddr,len);
						bzero(send_buff, MAX_DIM_MESSAGE);
						goto REQUEST;
					}
					
					/*Caso download*/
					else if(strncmp("3", send_buff, strlen("3")) == 0){
						printf("Client Default_port %d -> Richiesto download\n",port_client);
						printf("[FASE DOWNLOAD]\n");
						bzero(send_buff, MAX_DIM_MESSAGE);
						/*Invio la lista dei file al client*/
						f_lista(socketone,servaddr,len);
						f_download(socketone,servaddr,len);	
						bzero(send_buff, MAX_DIM_MESSAGE);
						goto REQUEST;
					}	
					/*Caso upload*/
					
					else if(strncmp("4", send_buff,strlen("4")) == 0){
						printf("Client Default_port %d -> RICHIESTO UPLOAD\n",port_client);
						printf("[FASE UPLOAD]\n");
						f_upload(socketone,servaddr,len);
						bzero(send_buff, MAX_DIM_MESSAGE);
						goto REQUEST;
					}
				}
				
			}	
		}
	}
	return 0;
}

void f_upload(int socketone, struct sockaddr_in servaddr, socklen_t len){	
	//svuoto il send_buff
	bzero(send_buff,MAX_DIM_MESSAGE);
	//metto la mia stringa nel send_buff
	sprintf(send_buff, "%s","Concesso l'upload.");
	//mando il mio send_buff alla socket del server
	sendto(socketone, send_buff, sizeof(send_buff), 0, (struct sockaddr *) &servaddr, len);
	reception_data();
}


void f_download(int socketone, struct sockaddr_in servaddr, socklen_t len){
	WAIT_FILE:
	number=0;
	/*Ricevo dal client il file richiesto*/
	bzero(send_buff, MAX_DIM_MESSAGE);
	
	
	// ricevo il messaggio dal client con il nome del file da aprire
	if(recvfrom(socketone, send_buff, MAX_DIM_MESSAGE,0, (struct sockaddr *) &servaddr, &len) < 0){
		if (errno==EAGAIN)
		{
			goto WAIT_FILE;
		}
		else{
			herror("Errore nella recvfrom della get_name_and_size_file del server.");
		}
		
	}
	printf("ERRNO -> %d",errno);
	
	
	
	// pulisco il send_buff dal terminatore di stringa
	int l = strlen(send_buff);
	if(send_buff[l-1] == '\n'){
		send_buff[l-1] = '\0';
	}
	
	int num_sequence=0;
	int fd = open(send_buff, O_RDONLY, 0666); /*FILE*/
    if(fd == -1){
    	printf("Attenzione! Ricevuto un nome sbagliato\n");
		/*Mando il nome*//*Oppure not_exist se non esiste*/
		if(sendto(socketone,"not_exist", MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, len) < 0) { 
			herror("Errore nella sendto della send_name del server.");
		}
		bzero(send_buff, MAX_DIM_MESSAGE);
		goto WAIT_FILE;
		
	}
	
	
	
	
	/*Mando il nome*//*Oppure not_exist se non esiste*/
	if(sendto(socketone,send_buff, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, len) < 0) { 
		herror("Errore nella sendto della send_name del server.");
	}
	
	/*Calcolo il numero di pacchetti da inviare*/
	size = lseek(fd, 0, SEEK_END);
    num_pacchetti = (ceil((size/Dimensione_pacchetto)))+1;
	printf("[INFORMAZIONI PRELIMINARI]------------------------------------------------\n");
    printf("Numero di pacchetti :%d\n",num_pacchetti);
	printf("Ho ricevuto un file di lunghezza %d\n",size);
 	lseek(fd, 0, 0);
 	// pulisco il send_buff e ci salvo la lunghezza del file
 	bzero(send_buff, MAX_DIM_MESSAGE);
	sprintf(send_buff, "%d", size);
	
	/*Mando la lunghezza del file*/
	if(sendto(socketone, send_buff, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, len) <0){
 		herror("Errore nella sendto della send_len_file del server.");
 	}	
	
	/*Invio i pacchetti*/
	
	/*Inizio il caricamento del file*/
	struct pacchetto file_struct[num_pacchetti];//creo tanti pacchetti quanti calcolati prima con num_pacchetti
	for(int i = 0; i < num_pacchetti; i++){
		bzero(file_struct[i].buf, MAX_DIM_MESSAGE);
	}
	/*Creo un send_buff avente la dimensione pari ad un pacchetto*/
	char *temp_buf;
	temp_buf = malloc(Dimensione_pacchetto);//temp_buf ha la dimensione di un pacchetto
	
	
	for (int i = 0; i < num_pacchetti; i++){//ciclo for ripetuto per tutti i pacchetti
			
		bzero(temp_buf, Dimensione_pacchetto);//pulisco tempo_buf
		int red=read(fd, temp_buf, Dimensione_pacchetto);//leggo quanto possibile da incapsulare in un pacchetto
		printf("Ho letto %d\n",red);
		/*Creo un array di dimensione pari ad un pacchetto*/
		char pacchetto[MAX_DIM_MESSAGE];
		
		/*Inserisco i dati di quel pacchetto*/
		sprintf(pacchetto, "%d ", i);//Copio il numero di sequenza di quel pacchetto
		strcat(pacchetto, temp_buf);//Copio nell'array creato per contenerlo, il numero di sequenza ed il contenuto precedentemente ricavato
		sprintf(file_struct[i].buf, "%s", pacchetto);//Riscrivo quello appena creato nella struttura nella posizione i-esima
		
		file_struct[i].numero_ordine = i;//assegno l'indice i-esimo all'entry in quella struttura
	}
	
	printf("[CONTENUTO FILE_STRUCT]------------------------------------------------\n");
	/*Stampo l'inter struttura*//*-> righa 299 client*/
	for (int i = 0; i < num_pacchetti; i++){
		printf("\nFILE_STRUCT[%d].BUF contiene :\n%s\n--------------------------------------------------------------\n",i,file_struct[i].buf);
	}
	
	/*Da qui in poi ho tutti i pacchetti salvati nella struttura*/
	printf("[PACCHETTI CARICATI CORRETTAMENTE NELLA STRUTTURA]\n\n");
	printf("[FASE DI SCAMBIO]---------------------------------------------------\n");
	/*Fase di invio dei pacchetti*/
	
	/*Vedo quanti pacchetti non sono multipli della windows dimensione*/
	int not_multiple = (num_pacchetti)%Dimensione_finestra;
	/*Caso in cui il numero dei pacchetti da inviare in maniera diversa perche non sono un multipli della WINDOWS_SIZE*/
	
	/*Se ci sono pacchetti "multipli di WINDOWS_SIZE" da inviare invio quelli*/
	if(num_pacchetti-num_sequence >= not_multiple){
		while(num_sequence<num_pacchetti - not_multiple){//fino a quando non sono arrivato al primo pack "NON multiplo di WINDOWS_SIZE"	
			num_sequence = GO_BACK_N_Send(file_struct, num_sequence, Dimensione_finestra);//mando la struttura contenente i pacchetti, la sequenza, e la dimensione della finestra
		}
		/*Una volta inviati i pacchetti "multipli di WINDOWS_SIZE" invio not_multiple pacchetti "NON multipli di WINDOWS_SIZE"*/
		if(num_sequence<num_pacchetti){		/*Nel caso ne rimangano alcuni, li invio*/
		num_sequence = GO_BACK_N_Send(file_struct, num_sequence, not_multiple);//mando la struttura contenente i pacchetti, la sequenza, e il numero di pacchetti rimanenti
		}
	}
	printf("[FINE FASE]----------------------------------------------------\n");
	FINISH:
	/*Reset delle informaizoni*/
	si=0;
	num_sequence=0;
	number=0;
	bzero(send_buff, MAX_DIM_MESSAGE);
}

/*
Il server in questo caso dovrebbe scartare i pacchetti 
fuori sequenza e mandare ack complessivi, oppure in caso di pacchetto fuori ordine l'ultimo ack riccevuto
*/
void UDP_GO_BACK_N_Recive(){
	received=0;
	number=0;
	/*
	Il server conoscendo la windows size, la dimensione per ogni pack, e la grandezza del file, può calcolarsi
	quante ondate di pacchetti "normali" e "diversi" arriveranno dal client
	*/
	int num_sequence,Timer; //dorvò scegliere opportunamente il Timer di scadenza
	int CONTATORE = 0;
	int counter = 0;
	struct pacchetto pacchett[num_pack];//Creo la struttura per contenere i pacchetti
	int not_multiple = num_pack%WINDOW_SIZE; //indica quante ondate di pacchetti devo ricevere 
	int w_size=WINDOW_SIZE;//w_size prende la dimensione dell WINDOW_SIZE per poi riadattarla in caso di pack "diversi"
	/*Vado nel ciclo finche non termino i pacchetti*/
	while(number < num_pack){
		/*Inizio a ricevere pacchetti*/
		/*
		Entriamo in questo ciclo solo nel caso in cui rimangono al piu not_multiple pacchetti, 
		di conseguenza la dimensione della nostra finestra diventa not_multiple
		 */
		if(num_pack-CONTATORE <= not_multiple + 1 && not_multiple!=0){
			if(WINDOW_SIZE%2){
				w_size = not_multiple;
			}
			else
			{
				w_size = not_multiple +1;
			}
		}
		/*
		1)w_size ha dimensione WINDOW_SIZE nel caso di pach "normali" 
		2)w_size ha dimensione not_multiple nel caso di pach "diversi" 
		*/
		for(int i = 0; i <w_size; i++){
			CICLO:
			bzero(send_buff, MAX_DIM_MESSAGE);
			printf(" ");
			char pacchetto_ricevuto[MAX_DIM_MESSAGE];
			char *contenuto_pacchetto;
			contenuto_pacchetto = malloc(Dimensione_pacchetto);
			
			/*attesa di un pacchetto*/
			int c_error = recvfrom(socketone, pacchetto_ricevuto, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);
			if (c_error < 0){
				if(errno == EAGAIN)
				{
					printf("buffo di ricezione scaduto nella recive_UDP_rel_file del client\n");
					goto FINE;
				}
				else
				{
					herror("c_errorore nella recvfrom della recive_UDP_rel_file nel client");
				}
			}
			// buff riceve il numero di sequenza messo nel header del pacchetto
			char *buff;
			const char s[2] = " ";
			buff = strtok(pacchetto_ricevuto, s);//divido il pacchetto in piu stringhe divise da s e lo metto in buf tutto segmentato
			int k = atoi(buff); //i prende il numero di sequenza nell'headewr del pacchetto
			num_sequence=k;//num_sequence prende il numero di sequenza nell'headewr del pacchetto
			if(num_sequence!=number){
				printf("Pacchetto %d ricevuto fuori ordine, ora lo scarto e rimando l'ack di %d\n",num_sequence,(received-1));
				if(SEND_ACK((received-1),WINDOW_SIZE)){//invio l'ack del pacchetto antecedente a quello che mi sarei aspettato
					goto CICLO;
				}
				else{
					goto NOOK;
				}
			
			}
			if(num_sequence >= CONTATORE){
				CONTATORE = num_sequence + 1;
			}
			printf("--------------------------------------[HO RICEVUTO IL PACCHETTO %d]------------------------------------\n",num_sequence);
			contenuto_pacchetto=parsed(num_sequence,pacchetto_ricevuto);

			
			printf("------------------------------[CONTENUTO PACK %d-ESIMO]------------------------------------\n%s\n------------------------------------------------------------------------------------\n",num_sequence,contenuto_pacchetto);
			/*Ora devo mandare gli ack */
			if(SEND_ACK(num_sequence,WINDOW_SIZE)){
			counter = counter + 1;
			received++;
				// copia del contenuto del pacchetto nella struttura ausiliaria
				if(strcpy(pacchett[num_sequence].buf, contenuto_pacchetto) == NULL){
					exit(-1);
				}
				if (strcpy(buff_file[num_sequence], pacchett[num_sequence].buf) == NULL){
					exit(-1);
				}
				printf("Pacchetto riscontrato numero di num_sequence: %d.\n\n", num_sequence);	
				//incremento il contatore che mi identifica se il pack è in ordine		
				number=num_sequence+1;
			}
			else{
				NOOK:
				printf("Pacchetto NON riscontrato numero di num_sequence: %d.\n", num_sequence);
				goto CICLO;
				
			}
			free(contenuto_pacchetto);
			FINE:
			printf(" ");	
			}		
	}
	received=0;
	return;
}


char * parsed(int num_sequence, char pacchetto_ricevuto[]){
		char *dime;
		dime = malloc(Dimensione_pacchetto+8);
		sprintf(dime, "%d", num_sequence);
		int st = strlen(dime) + 1;
		char *start = &pacchetto_ricevuto[st];
		char *end = &pacchetto_ricevuto[MAX_DIM_MESSAGE];
		char *string = (char *)calloc(1, end - start + 1);
		memcpy(string, start, end - start);
		free(dime);
		return string;
}

/*
Funzione per l'invio dell'ACK;prendo in input il numero di sequenza del pacchetto
per cui voglio dare un riscontro
*/
int SEND_ACK(int num_sequence,int WINDOW_SIZE){
	int loss_prob;
	printf("Sto per inviare il risocntro del pacchetto [%d]\n",num_sequence);
	bzero(send_buff, MAX_DIM_MESSAGE);
	sprintf(send_buff, "%d", num_sequence);
	if(num_sequence > num_pack-WINDOW_SIZE-1)
	{
		loss_prob = 0;
	}
	else
	{
		loss_prob = Perc_perdita;
	}
	int random = num_random(); 
	if(random < (100 - loss_prob)) {
		c_error = sendto(socketone, send_buff, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, len);
		bzero(send_buff, MAX_DIM_MESSAGE);
		if(c_error < 0){
			herror("c_errorore nella sendto della SEND_ACK del server.");
		}
		return 1;
	}
	else
	{
		bzero(send_buff, MAX_DIM_MESSAGE);
		return 0;
	}
}



/*Questa funzione serve per creare numeri random da 0 a 100*/
int num_random(){
	return rand()%100;
}
/*Questa funzione serve per chiudere una socket*/
int chiudi_socket(int sock){
	return close(sock);
}

/*
Questa funzione serve a preparare il server alla ricezione dei dati allocando memoria sufficente per contenerli
In particolare riceve informaizoni come il nome, la dimensione
ed in base ad esse si calcola il numero di pacchetti da ricevere
In base al nome ricevuto essa crea un file dello stesso nome all'interno della repository
e alla fine della ricezione copia l'intero contenuto dei pack ricevuti su quel file
In fine aggiorna la lista dei file disponibili sul server
*/
void reception_data(){
	START_RECEIVE_LEN:
	bzero(send_buff, MAX_DIM_MESSAGE);
	/*Ricevo il nome del file*/
	c_error = recvfrom(socketone, send_buff, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);
	if(c_error < 0){
		if(errno == EAGAIN)
		{
			goto START_RECEIVE_LEN;
		}
		herror("c_errorore nella recvfrom della receive_name_and_len_file del server.");
	}
	/*Lo copio in un send_buff*/
	if(strcpy(pathname, send_buff) == NULL){
		herror("c_errorore nella strncpy della receive_name_and_len_file del server.");
	}
	bzero(send_buff, MAX_DIM_MESSAGE);
	/*
	Ora per ricevre il file devo allocare una memoria sufficentemente grande per contenerlo
	ogni volta che mi arriva un pacchetto devo far si che la memoria si ricordi qual'era il precedente, e quanti ne mancano
	*/
	/*Attendo la lunghezza del file*/
	c_error = recvfrom(socketone, send_buff, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);
	if(c_error < 0){
		herror("c_errorore nella recvfrom della receive_len_file del server.");
	}
	int dim_file=atoi(send_buff);
	printf("Ho ricevuto un file di lunghezza :%d\n",dim_file);
	/*Alloco la memoria per contenerlo*/
	buff_file=malloc(dim_file);
	/*
	Mediante la chiamata ceil:
	La funzione restituisce il valore integrale più piccolo non inferiore a x .
	*/
	num_pack = (ceil((dim_file/Dimensione_pacchetto)))+1;
	printf("Numero pacchetti da ricevere: %d.\n", num_pack);
	/*
	Utilizzo questa tecnica per capire quanti pacchetti dovrò ricevere
	Alla fine della procedura avro a disposizione sia il nome del file e la sua lunghezza
	*/
   	for(int i = 0; i < num_pack; i++){
    	buff_file[i] = mmap(NULL, Dimensione_pacchetto, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
    	if(buff_file[i] == NULL){
			herror("c_errorore nella mmap del buff_file della receive_name_and_len_file del server.");
		}
    }
    
	/*Copio il contenuto in un nuovo file man mano che ricevo pacchetti*/
	int file = open(pathname, O_CREAT|O_RDWR, 0666);
	if(file == -1){
		herror("c_errorore nella open in create_local_file del server.");
	}
	printf("Inizio a ricevere i pacchetti con GO-BACK-N\n");
	UDP_GO_BACK_N_Recive();	
	printf("Ricezione terminata correttamente.\n");
	
	
	/*Funzione che riscrive l'intera struttura nel file*/
	for(int i = 0; i < num_pack; i++){
		
		int ret = write(file, buff_file[i], strlen(buff_file[i]));
		if(ret == -1){
			herror("c_errorore nella write della write_data_packet_on_local_file del server.");
		}
		
	}
	printf("File scritto correttamente.\n");
	/*Aggiorno la lista dei file*/
	FILE *f;
	f=fopen("lista.txt","a");
	fprintf(f, "\n%s", pathname); 
	exist=0;
	printf("File list aggiornato correttamente.\n[OPERAZIONE COMPLETATA CON SUCCESSO]\n");
	fclose(f);
	return;
}
	

/*
Questa funzione viene utilizzata per creare socket
Viene creata una socket e la struct di supporto
Viene ritornata la socket
*/
int creazione_socket(int s_port){
	printf("Creazione socket:\n");
	// creazione della socket
	int s_socketone = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// salvo in len la lunghezza della struct della socket
	len = sizeof(servaddr);
	// controllo d'c_errorore nella creazione della socket
	if(s_socketone == -1){
		herror("ATTENZIONE! Creazione della socket fallita...");
	}
	else{
		printf("Socket creata\n");
	}
	// pulisco la memoria allocata per la struttura della socket
	bzero(&servaddr,sizeof(servaddr));
	// setto i parametri della struttura della socket
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	//servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	servaddr.sin_port=htons(s_port);

	// binding della socket con controllo d'c_errorore
	if((bind(s_socketone, (struct sockaddr *)&servaddr, sizeof(servaddr)))!=0){
		herror("ATTENZIONE! Binding della socket fallito...");
	}
	else{
		printf("Socket-Binding eseguito\n");
	}
	return s_socketone;
}

/*
Questa funzione viene utilizzata quando il client richiede la lista dei file salavti in memoria
Viene aperto un canale di comunicazione di sola lettura verso il file
Vine effettuata una scansione mediante l'indice di lettura e scrittura che mi restituirà la grandezza specifica del file
Poi viene allocata una quantità di memoria sufficente a contenere l'intero contenuto del file
Viene copiato il contenuto del file in tale memoria
Viene passato l'informazione alla socket verso il client contenente la lista dei file
*/
void f_lista(int socketone, struct sockaddr_in servaddr, socklen_t len){
	int fd; //Puntatore al file contenente la lista dei file
	fd= open("lista.txt",O_RDONLY,0666);//apro uno stream di sola lettura verso il file
	if(fd==-1){
		printf("c_errorore apertura lista dei file\n");
		return;
	}
	size = lseek(fd,0,SEEK_END); //Vedo la dimensione del file
	if(size<0){
		printf("c_errorore lettura della dimensione della lista dei file\n");
		return;
	}
	buff_file_list=malloc(size); //alloco la memoria per contenerlo
	if(buff_file_list==NULL){
		printf("c_errorore allocazione memoria per contenere la lista dei file\n");
		return;
	}
	lseek(fd,0,0);//riposiziono la testina all'inizio del file
	while((read(fd,buff_file_list,size)==-1)){//inserisco all'interno di buff_file_list l'intero contenuto del file
		if(errno!=EINTR){
			printf("c_errorore lettura contenuto della lista dei file\n");
			return;
		}
	}
	while((sendto(socketone,buff_file_list,size,0,(struct sockaddr *) &servaddr, len))==-1){//metto il contenuto sulla socket
		if(errno!=EINTR){
		printf("c_errorore caricamneot lista dei file sulla socket\n");
		return;
		}
	}	
	free(buff_file_list);//dealloco la memoria allocata precedentemente con la malloc
}


/*
Questa funzione viene utilizzata quando il child decide di uscire
Il server prende come dati il pid e la port_client inerente al client connesso
Medianti questi dati lancia un segnale di kill verso quel process ID e chiude la socket
*/
void f_esci(int port_client, int socket_fd, pid_t pid){
	printf("Chiudo la connessione verso la porta: %d.\n", port_client);
	int ret = chiudi_socket(socket_fd);
	if(ret == -1){
		herror("c_errorore nella chiusura della socket\n");
	}
	*numeri_di_porta[port_client-Default_port-1]=port_client;
	*utenti_connessi=decrement_client(utenti_connessi);
	exit(1);
	
}
	




//Funzione di terminazione dei child
void *exit_t(){
	printf("\n");
	bzero(send_buff, MAX_DIM_MESSAGE);
	sprintf(send_buff, "%d", Code);
	/*Comunico al client la chiusura*/
	if(sendto(socketone, send_buff, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, len) < 0){ 
		herror("c_errorore invio segnale di chiusura della socket al child.");
	}
	/*Chiudo la socket del child*/
	chiudi_socket(socketone);
	printf("socketone (socket di gestione verso il client) chiusa con successo\n");
	
	/*Avviso il padre della chiusura*/
	kill(parent,SIGINT); //invio al padre il segnale SIGINT
	
	exit(1);
}


//funzione di terminazione del parent
void *exit_p(){
	printf("\n");
	printf("Numero utenrti connessi %d\n",*utenti_connessi);
	if(*utenti_connessi>0){
		esci();
	}
	chiudi_socket(s_socketone);
	printf("s_socketone (socket di accoglienza) chiusa con successo\n");
	exit(1);
}

/*Funzione per comunicare al client che le porte sono piene*/
void* esci(){
	bzero(send_buff, MAX_DIM_MESSAGE);
	//scrivo il valore aggionrato nel send_buff di comunicazione
	sprintf(send_buff,"%d",Porta_chiusura);
	//comunico al client che sono pieno
	if(sendto(s_socketone, send_buff, MAX_DIM_MESSAGE,0, (struct sockaddr *) &servaddr, len) < 0){
		herror("c_errorore nella sendto 2 del primo while del main del server.");
	}
	bzero(send_buff, MAX_DIM_MESSAGE);

}

/*Questa funzioen serve per mandare dei messaggi con un algoritmo scelto (GO-BACK-N) 
La funzione manda preliminarmente tutti i pacchetti dentro la window dimensione(finestra di trasmissione)
per poi mettersi in attesa dei loro riscontri, facendo scorrere la finestra ogni qual volta arriva un ack.
La funzione associa il Timer al primo pacchetto della finestra, ed ogni volta che esso viene riscontrato viene fatto
partire il Timer associato al nuovo leader della finestra(il primo)	*/
int GO_BACK_N_Send(struct pacchetto *file_struct, int num_sequence, int not_multiple){//not_multiple ha il valore della WINDOWS dimensione per i pacchetti "multipli di windows dimensione" e not_multiple per i pack "non multipli"
	int i;
	int j=0;
	si = 0; //indice di partenza per riscontrar ei pack
	int Timer=1; //utile per far partire il Timer solo del primo pacchetto
	printf("\n--------------------------------------------------------------------------------------------------------\n");
	/*Ciclo for che invia not_multiple pacchetti alla volta*/
	for(i = 0; i < not_multiple; i++){	
		if(num_sequence+i>=num_pacchetti){
				goto WAIT;
				
			}
		//imposto l'ack del pacchetto che sto inviando come 0, lo metterò a 1 una volta ricevuto l'ack dal client
		/*num_sequence(inzialmente uguale a 0), indica il numero del pack*/
		file_struct[num_sequence+i].ack = 0;
		/*Mando il primo pack*/
		if(sendto(socketone, file_struct[num_sequence+i].buf, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, len) < 0) { 
			fprintf(stderr, "Errore nell'invio del pacchetto numero: %d.\n", num_sequence);
			exit(-1);
		}
<<<<<<< HEAD
		printf("Pacchetto [%d] inviato\n",num_sequence+i);
		if(Timer==1){
			/*Qui parte il Timer associato al pack num_sequence*/
			setTimeout(Timer,num_sequence+i);//timeout del primo pack
=======
		printf("Pacchetto [%d] inviato\n",seq+i);
		if(timer==1){
			/*Qui dovrebbe partire il timer associato al pack seq*/
			setTimeout(SEND_FILE_TIMEOUT,seq+i);//timeout del primo pack
>>>>>>> parent of 27f9b1d... Update #1 #2 #4
		}
		Timer=0;
		bzero(send_buff, MAX_DIM_MESSAGE);
		
	}
	printf("\n--------------------------------------------------------------------------------------------------------\n");
	/*Entro nella fase di attesa dei riscontri dei pacchetti*/
	int seq2=num_sequence;
	for(j = si; j < not_multiple; j++){
		WAIT:
		if(num_sequence>=num_pacchetti){
			goto FINE;
		}
		printf("Attendo ACK [%d]\n",num_sequence+si);
		bzero(send_buff, MAX_DIM_MESSAGE);
		int err = recvfrom(socketone,send_buff, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);//Vado in attesa del riscontro da parte del server
		errno=0;
		/*Caso perdo il pacchetto*/
		if (err < 0){
			if(errno == EAGAIN){
				/*
				Nel caso viene perso un pacchetto mi sposto di nuovo dentro il ciclo for, permettendo di nuovo l'invio dei pacchetti non riscontrati
				*/
				printf("Il pacchetto è andato perso o danneggiato, ack: %d non ricevuto\n\n\n------------------------------------------------------------------------\n",num_sequence);
				i=id;//Utile per impostare l'indice nel ciclo for, indica l'id del pacchetto di cui mi aspetto un riscontro
				si=id;//Utile per impostare l'attesa degli ack dei pacchetti ritrasmessi
				num_sequence=num_sequence-i;
				bzero(send_buff, MAX_DIM_MESSAGE);
				}
		
			else{
				printf("Timer scaduto...\n");
				
			}
		}
		/*Caso prendo il pacchetto*/
		else{
			int check = atoi(send_buff);//Prendo l'id del pacchetto riscontrato dal server
			bzero(send_buff, MAX_DIM_MESSAGE);
			if(check >= 0){
				file_struct[check].ack = 1; //Imposto l'ack del pacchetto all'interno della struttura uguale a 1, indicando che tale pack è stato riscontrato
			}
			if(check!=number){//caso ack diverso da quello che mi aspettavo
				printf("Ho ricevuto un ack del pacchetto %d, ma mi aspettavo %d\n\n",check,number);
				id++;
			}
			else{//caso ack uguale a quello aspettato
				printf("Ho ricevuto l'ack del pacchetto [%d]\n\n",check);
				setTimeout(Timer,num_sequence+si);//timeout per i pack successivi al primo
				
				/*
				Questo controllo su num_sequence serve per far scorrere l'id del pacchetto ricercato in mainera ottimale
				In particolare andrà ad indicare quale sarà il prossimo pacchetto da riscontrare:
				- Se num_sequence è uguale a 0 indica che ho riscontrato il pacchetto 0-esimo e che mi aspetto il pacchetto 1-esimo
				- Nel resto dei casi si passa dal pacchetto i-esimo a quello (i+1)-esimo
				*/
				if(num_sequence==0){
					num_sequence=num_sequence+check+1; //Avviene solo nella trasmissione del primo pacchetto, server per indicare quale sarà il prossimo pack da riscontrare
				}
				else{
					num_sequence=check+1; //Quello successivo a quello riscontrato correttamente, cioè quello da inviare di nuovo in caso di perdita
					}		
				/*Faccio ripartire il Timer al pack num_sequence*/
				number++;
				id++;
			}
		}
	}

	FINE:
	return num_sequence;
}

/*Questa funzione serve per settare un tempo alla richiesta della socket*/
void setTimeout(double time,int id) {
	struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = time;
    setsockopt(socketone, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}
	
	
		