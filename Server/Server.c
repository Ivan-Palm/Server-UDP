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
#include<errno.h>


/*Valori definiti preliminarmente*/
#define PORTA_DI_CHIUSURA 8089 //Usata per creare una soket di comunicazione al client che dorvà essere subito scollegato
#define PORT 8090 //Porta di default per l'inizio delle conversazioni client-server
#define MAXDIM 1024
#define CODICE 25463 //Codice di utility per gestire la chiusura del server
#define CONNESSIONI 5 //Numero massimo di connessioni accettate dal server
#define MAX_DIM_MESSAGE 1024 //Diensione totale del messaggio che inviamo nell'applicativo
#define SA struct sockaddr //Struttura della socket
#define DIM_PACK 1024 // dimensione del payload nel pacchetto UDP affidabile
#define L_PROB 15 // probabilità di perdita



/*Dichiarazioni funzioni*/
void f_esci(int , int , pid_t);
void f_lista(int, struct sockaddr_in, socklen_t);
void *exit_t();
void *esci();
void child_exit();
void f_upload(int , struct sockaddr_in , socklen_t );
void reception_data();
int sendACK(int ,int );
int creazione_socket(int);
int num_random();
int chiudi_socket(int);
void show_port();
int decrement_client(int*);
void *exit_p();


/*strutture*/
struct pacchetto{
	int position;//indica la posizione
	char buf[MAX_DIM_MESSAGE];//dati
	int ack;//indica se è un riscontro
};


/*Variabili globali*/
int WINDOW_SIZE=3;
int receive=0; //indica l'ultimo pacchetto ricevuto correttamente
int num_pack; 	// numero di pacchetti da inviare
char **buff_file;
char pathname[1024];
int num=0;//contatore utile per verificare l'ordine dei pacchetti
int **numeri_di_porta;
int *utenti_connessi;
int num_port[CONNESSIONI];
void *exit_t();
char *buff_file_list; //Buffer per il contenuto della lista di file
char buffer[MAX_DIM_MESSAGE]; //Buffer per comunicare con i client
int s_socketone;//File descriptor della socket per i child
pid_t parent_pid; //PID del parent nel main
int size; //Dimensione del file da trasferire
struct sockaddr_in servaddr;//Struct di supporto della socket
socklen_t len;//Lunghezza della struct della socket
int port_number = 0; //Variabile di utility per il calcolo delle porte successive da dare al client
int shmid; 	//Identificativo della memoria condivisa
int port_client; //Porta che diamo al client per le successive trasmissioni multiprocesso
int socketone;	//File descriptor di socket
int c_error; 	// intero per il controllo della gestione d'c_errorore
int parent; //pid del processo padre



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
		*numeri_di_porta[i]=i+1+PORT;
	}
	printf("\n-------------------------Porte disponibili:-------------------------\n");
	for(int l=0;l<CONNESSIONI;l++){
			printf("\t\t\t\t[%d]\n",*numeri_di_porta[l]);
	}
	
	/*Inizializzo la sharedmemory per salvare i process-id dei child*/
	shmid = shmget(IPC_PRIVATE, sizeof(int)*CONNESSIONI, IPC_CREAT|0666);
	if(shmid == -1){
		herror("c_errorore nella shmget nel main del server.");
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
	//signal(SIGCHLD,(void*)exit_p);//esco nel caso di cntl+c
	show_port();
	define_num_client();
	/*Salvo il pid del processo padre in una variabile globale*/
	parent_pid = getpid();
	
	//creo la socket di comunicazione per i child
	s_socketone = creazione_socket(PORT);
	
	
	/*//creo un processo che gestisce l'eventuale richiesta di chiusura del server-->1° Child creato per la gestione della chiusura
	pid_t pid = fork();
	if(pid == 0){
		signal(SIGINT,(void*)exit_t); //Se viene premuto ctlt+c viene passato il controllo ad esci e viene inviato al padre il segnale di uscita
	}
	parent = pid; //il padre prende il pid parent
	*/
	
	
	//entro nel ciclo infinito di accoglienza di richieste
	//imposto i segnali
	
	
	while(1){
		bzero(buffer, MAX_DIM_MESSAGE);
		//attendo un client
		if(recvfrom(s_socketone, buffer, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, &len) < 0){
			herror("c_errorore nella recvfrom nel primo while del server.");
		}

		bzero(buffer, MAX_DIM_MESSAGE);
	
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
			port_client = PORT + port_number;//il primo avra 8091 il secondo 8092 e cosi via...
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
			printf("\n------------------------------NUOVO UTENTE CONNESSO!Client port %d------------------------------\n",port_client);
			bzero(buffer, MAX_DIM_MESSAGE);
			//scrivo il valore aggionrato nel buffer di comunicazione
			sprintf(buffer,"%d",port_client);
			//comunico al client su quale porta si sta connettendo
			if(sendto(s_socketone, buffer, MAX_DIM_MESSAGE,0, (struct sockaddr *) &servaddr, len) < 0){
				herror("c_errorore nella sendto 2 del primo while del main del server.");
			}
			bzero(buffer, MAX_DIM_MESSAGE);
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
				//signal(SIGCHLD, SIG_IGN);
				//signal(SIGUSR1, SIG_IGN);
				//signal(SIGUSR2, child_exit);//Gestione della chiusira del server, manda un messaggio di chiusura verso il client
				/*Entro nel ciclo di ascolto infinito*/
				while(1){
					bzero(buffer, MAX_DIM_MESSAGE);//Pulisco il buffer
					/*Vado in attesa di un messaggio*/
					if(recvfrom(socketone, buffer, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, &len) < 0){
						if (errno==EAGAIN)
						{
							return 0;
						}
						else{
							herror("c_errorore nella recvfrom del secondo while del main del server.");
						}
					}
					/*Gestisco la richiesta del client*/
					/*Caso exit*/
					if(strncmp("1", buffer, strlen("1")) == 0){
						printf("Client port %d -> Richiesto exit\n",port_client);
						f_esci(port_client,socketone,parent_pid);
						while(1){
							sleep(1000);
						}
						bzero(buffer, MAX_DIM_MESSAGE);
					}
					
					/*Caso list*/
					else if(strncmp("2", buffer, strlen("2")) == 0){
						printf("Client port %d -> Richiesto list\n",port_client);
						f_lista(socketone,servaddr,len);
					}
					
					/*Caso download*/
					else if(strncmp("3", buffer, strlen("3")) == 0){
						printf("Client port %d -> Richiesto download\n",port_client);
						//func_download(socketone,servaddr,len);
						bzero(buffer, MAX_DIM_MESSAGE);
						
					}	
					/*Caso upload*/
					
					else if(strncmp("4", buffer,strlen("4")) == 0){
						printf("Client port %d -> Richiesto upload\n",port_client);
						f_upload(socketone,servaddr,len);
						bzero(buffer, MAX_DIM_MESSAGE);
					}
					
					/*Caso c_errorore*/
					else{
						//func_c_erroror(socketone,servaddr,len);
					}
				}
				
			}	
		}
	}
	return 0;
}

void f_upload(int socketone, struct sockaddr_in servaddr, socklen_t len){	
	//svuoto il buffer
	bzero(buffer,MAX_DIM_MESSAGE);
	//metto la mia stringa nel buffer
	sprintf(buffer, "%s","Concesso l'upload.");
	//mando il mio buffer alla socket del server
	sendto(socketone, buffer, sizeof(buffer), 0, (struct sockaddr *) &servaddr, len);
	reception_data();
}

/*
Il server in questo caso dovrebbe scartare i pacchetti 
fuori sequenza e mandare ack complessivi, oppure in caso di pacchetto fuori ordine l'ultimo ack riccevuto
*/
void recive_UDP_GO_BACK_N(){
	receive=0;
	num=0;
	/*
	Il server conoscendo la windows size, la dimensione per ogni pack, e la grandezza del file, può calcolarsi
	quante ondate di pacchetti "normali" e "diversi" arriveranno dal client
	*/
	int seq,timer; //dorvò scegliere opportunamente il timer di scadenza
	int count = 0;
	int counter = 0;
	struct pacchetto pacchett[num_pack];//Creo la struttura per contenere i pacchetti
	int offset = num_pack%WINDOW_SIZE; //indica quante ondate di pacchetti devo ricevere 
	int w_size=WINDOW_SIZE;//w_size prende la dimensione dell WINDOW_SIZE per poi riadattarla in caso di pack "diversi"
	/*Vado nel ciclo finche non termino i pacchetti*/
	while(count < num_pack||counter<num_pack-1){
		/*Inizio a ricevere pacchetti*/
		/*
		Entriamo in questo ciclo solo nel caso in cui rimangono al piu offset pacchetti, 
		di conseguenza la dimensione della nostra finestra diventa offset
		 */
		if(num_pack-count <= offset + 1 && offset!=0){
			if(WINDOW_SIZE%2){
				w_size = offset;
			}
			else
			{
				w_size = offset +1;
			}
		}
		/*
		1)w_size ha dimensione WINDOW_SIZE nel caso di pach "normali" 
		2)w_size ha dimensione offset nel caso di pach "diversi" 
		*/
		printf("\nW_SIZE: %d\n\n",WINDOW_SIZE);
		for(int i = 0; i <w_size; i++){
			CICLO:
			bzero(buffer, MAX_DIM_MESSAGE);
			printf(" ");
			char pckt_rcv[MAX_DIM_MESSAGE];
			char *pckt_rcv_parsed;
			pckt_rcv_parsed = malloc(DIM_PACK);
			
			/*attesa di un pacchetto*/
			int c_error = recvfrom(socketone, pckt_rcv, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);
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
			buff = strtok(pckt_rcv, s);//divido il pacchetto in piu stringhe divise da s e lo metto in buf tutto segmentato
			int k = atoi(buff); //i prende il numero di sequenza nell'headewr del pacchetto
			seq=k;//seq prende il numero di sequenza nell'headewr del pacchetto
			printf("\n\nMI ASPETTO PACK %d\n\n",num);
			if(seq!=num){
				printf("Pacchetto %d ricevuto fuori ordine, ora lo scarto e rimando l'ack di %d\n",seq,(receive-1));
				if(sendACK((receive-1),WINDOW_SIZE)){//invio l'ack del pacchetto antecedente a quello che mi sarei aspettato
					goto CICLO;
				}
				else{
					goto NOOK;
				}
			
			}
			
			
			if(seq >= count){
				count = seq + 1;
			}
			printf("\t\t\t\tHo ricevuto il pacchetto %d\n",seq);
			 //indico che è lui il nuovo pacchetto ricevuto
			/*
			La funzione restituisce la sottostringa del pacchetto -> PASSA MALE IL CONTENUTO
			contentente il messaggio vero e proprio
			*/
			char *c_index;
			sprintf(c_index, "%d", seq);
			int st = strlen(c_index) + 1;
			char *start = &pckt_rcv[st];
			char *end = &pckt_rcv[MAX_DIM_MESSAGE];
			char *substr = (char *)calloc(1, end - start + 1);
			memcpy(substr, start, end - start);
			pckt_rcv_parsed = substr;//pckt_rcv_parsed ha la stringa del pacchetto
			printf("----------------------------------------------------------CONTENUTO PACK %d-esimo:----------------------------------------------------\n%s\n:------------------------------------------------------------------------------------------------------------------------------------------\n",seq,pckt_rcv_parsed);
			
			/*Ora devo mandare gli ack */
			if(sendACK(seq,WINDOW_SIZE)){
			counter = counter + 1;
			receive++;
				// copia del contenuto del pacchetto nella struttura ausiliaria
				if(strcpy(pacchett[seq].buf, pckt_rcv_parsed) == NULL){
					exit(-1);
				}
				if (strcpy(buff_file[seq], pacchett[seq].buf) == NULL){
					exit(-1);
				}
				printf("Pacchetto riscontrato numero di seq: %d.\n\n", seq);	
				//incremento il contatore che mi identifica se il pack è in ordine		
				num=seq+1;
			}
			else{
				NOOK:
				printf("Pacchetto NON riscontrato numero di seq: %d.\n", seq);
				goto CICLO;
				
			}
			FINE:
			printf(" ");	
			}		
	}
	return;
}


/*
Funzione per l'invio dell'ACK;prendo in input il numero di sequenza del pacchetto
per cui voglio dare un riscontro
*/
int sendACK(int seq,int WINDOW_SIZE){
	int loss_prob;
	bzero(buffer, MAX_DIM_MESSAGE);
	sprintf(buffer, "%d", seq);
	if(seq > num_pack-WINDOW_SIZE-1)
	{
		loss_prob = 0;
	}
	else
	{
		loss_prob = L_PROB;
	}
	int random = num_random(); 
	printf("Numero random scelto: %d\n",random);
	if(random < (100 - loss_prob)) {
		printf("Sto inviando l'ACK: %d.\n", seq);
		c_error = sendto(socketone, buffer, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, len);
		bzero(buffer, MAX_DIM_MESSAGE);
		if(c_error < 0){
			herror("c_errorore nella sendto della sendACK del server.");
		}
		return 1;
	}
	else
	{
		bzero(buffer, MAX_DIM_MESSAGE);
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
	printf("Avviata procedura di ricezione del file\n");
	bzero(buffer, MAX_DIM_MESSAGE);
	/*Ricevo il nome del file*/
	c_error = recvfrom(socketone, buffer, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);
	if(c_error < 0){
		if(errno == EAGAIN)
		{
			goto START_RECEIVE_LEN;
		}
		herror("c_errorore nella recvfrom della receive_name_and_len_file del server.");
	}
	/*Lo copio in un buffer*/
	if(strcpy(pathname, buffer) == NULL){
		herror("c_errorore nella strncpy della receive_name_and_len_file del server.");
	}
	bzero(buffer, MAX_DIM_MESSAGE);
	/*
	Ora per ricevre il file devo allocare una memoria sufficentemente grande per contenerlo
	ogni volta che mi arriva un pacchetto devo far si che la memoria si ricordi qual'era il precedente, e quanti ne mancano
	*/
	/*Attendo la lunghezza del file*/
	c_error = recvfrom(socketone, buffer, MAX_DIM_MESSAGE, 0, (SA *) &servaddr, &len);
	if(c_error < 0){
		herror("c_errorore nella recvfrom della receive_len_file del server.");
	}
	int dim_file=atoi(buffer);
	printf("Ho ricevuto un file di lunghezza :%d\n",dim_file);
	/*Alloco la memoria per contenerlo*/
	buff_file=malloc(dim_file);
	/*
	Mediante la chiamata ceil:
	La funzione restituisce il valore integrale più piccolo non inferiore a x .
	*/
	num_pack = (ceil((dim_file/DIM_PACK))) + 1;
	printf("Numero pacchetti da ricevere: %d.\n", num_pack);
	/*
	Utilizzo questa tecnica per capire quanti pacchetti dovrò ricevere
	Alla fine della procedura avro a disposizione sia il nome del file e la sua lunghezza
	*/
   	for(int i = 0; i < num_pack; i++){
    	buff_file[i] = mmap(NULL, DIM_PACK, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
    	if(buff_file[i] == NULL){
			herror("c_errorore nella mmap del buff_file della receive_name_and_len_file del server.");
		}
    }
    
	/*Copio il contenuto in un nuovo file man mano che ricevo pacchetti*/
	int file = open(pathname, O_CREAT|O_RDWR, 0666);
	if(file == -1){
		herror("c_errorore nella open in create_local_file del server.");
	}
	printf("Ho creato il file.\n");
	printf("Inizio a ricevere i pacchetti con GO-BACK-N\n");
	recive_UDP_GO_BACK_N();	
	printf("Ricezione terminata correttamente.\n");
	
	/*penspo che manca la scrittur nel file fd*/
	
	printf("Scrivo il file...\n");
	for(int i = 0; i < num_pack; i++){
		int ret = write(file, buff_file[i], DIM_PACK);
		if(ret == -1){
			herror("c_errorore nella write della write_data_packet_on_local_file del server.");
		}
	}
	printf("File scritto correttamente.\n");
	/*Aggiorno la lista dei file*/
	printf("Aggiorno file_list...\n");
	FILE *f;
	f=fopen("lista.txt","a");
	fprintf(f, "\n%s", pathname); 
	printf("File aggiornato correttamente.\nOperazione di upload completata con successo.\n");
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
	*numeri_di_porta[port_client-PORT-1]=port_client;
	*utenti_connessi=decrement_client(utenti_connessi);
	kill(pid, SIGUSR1);
	kill(getpid(),0);	
}
	




//Funzione di terminazione dei child
void *exit_t(){
	printf("\n");
	bzero(buffer, MAX_DIM_MESSAGE);
	sprintf(buffer, "%d", CODICE);
	/*Comunico al client la chiusura*/
	if(sendto(socketone, buffer, MAX_DIM_MESSAGE, 0, (struct sockaddr *) &servaddr, len) < 0){ 
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
	printf("s_socketone (socket di accoglienza) chiusa con successo\n");
	chiudi_socket(s_socketone);
	exit(1);
}

/*Funzione per comunicare al client che le porte sono piene*/
void* esci(){
	bzero(buffer, MAX_DIM_MESSAGE);
	//scrivo il valore aggionrato nel buffer di comunicazione
	sprintf(buffer,"%d",PORTA_DI_CHIUSURA);
	//comunico al client che sono pieno
	if(sendto(s_socketone, buffer, MAX_DIM_MESSAGE,0, (struct sockaddr *) &servaddr, len) < 0){
		herror("c_errorore nella sendto 2 del primo while del main del server.");
	}
	bzero(buffer, MAX_DIM_MESSAGE);
}


	
	
		