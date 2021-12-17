#include <sys/types.h> /* Cabeceras de Sockets */
#include <sys/socket.h>
#include <netinet/in.h> /* Cabecera de direcciones por red */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h> /* Cabecera de Threads */
#include <signal.h> /* Cabecera de Signal */

#include "tablahash.h" /* Cabecera de Diccionario para los nicknames */


/* Macros */
#define MAX_CLIENTS 25
#define MAXMSG 1024
#define MAX_COMANDO 11
#define MAX_NICK 32

/* Variables globales: */
/* diccionario de nicknames */
TablaHash *diccionario;
/* socket del servidor */
int sock;
/* lock para la sección crítica */
pthread_mutex_t lock;

/* Anunciamos el prototipo del hijo */
void *rutina(void *arg);

/* Definimos una función auxiliar de error */
void error(char *msg) {
    perror(msg);
    exit(1);
}

/* Funciones de hash para nuestro diccionario de nicknames */
unsigned hash(char *s){
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
      hashval = *s + 11 * hashval;
    return hashval;
}
unsigned hash2(char *s){
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
      hashval = *s + 31 * hashval;
    return hashval;
}

/* Función que implementa el protocolo de salida */
void exit_protocol() {
    if (diccionario) {
        for (unsigned int i = 0; i < diccionario->capacidad; i++) {
            if (diccionario->tabla[i].dato && (diccionario->tabla[i].estado != 2)){
                send(*(int *)(diccionario->tabla[i].dato), "/TERM_PROCESS", sizeof("/TERM_PROCESS"), 0);
            }
        }
        tablahash_destruir(diccionario);
    }
    if (sock)
        close(sock);
}

/* Función de manejo de señales de terminación */
void salida_elegante() {
    exit_protocol();
    printf("Salí elegantemente\n");
    exit(0);
}



/* Asumimos que el primer argumento es el puerto por el cual escuchará nuestro servidor */
int main(int argc, char **argv){

    /* Manejo de salida*/
    signal(SIGTERM, salida_elegante);
    signal(SIGINT, salida_elegante);
    
    int *soclient;
    struct sockaddr_in servidor, clientedir;
    socklen_t clientelen;
    pthread_t thread;
    pthread_attr_t attr;
    diccionario = tablahash_crear(31, hash, hash2);
    
    /* Chequeamos que los argumentos fueron pasados */
    if (argc <= 1) {
        exit_protocol();
        error("Faltan argumentos");
    }

    /* Creamos el socket */
    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        exit_protocol();
        error("Falló el Socket Init");
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    
    /* Creamos a la dirección del servidor */
    servidor.sin_family = AF_INET; /* Internet Ipv4 */
    servidor.sin_addr.s_addr = INADDR_ANY; /**/
    servidor.sin_port = htons(atoi(argv[1])); /* Puerto */

    /* Inicializamos el socket */
    if (bind(sock, (struct sockaddr *) &servidor, sizeof(servidor))) {
        exit_protocol();
        error("Error en el bind");
    }
    printf("Binding exitoso, esperando conexiones en %s\n",argv[1]);

    /* Creamos los atributos para los hilos */
    pthread_attr_init(&attr);
    /* Hilos que no van a ser *joinables* */
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    /* Ya podemos aceptar conexiones */
    if (listen(sock, MAX_CLIENTS) == -1) {
        exit_protocol();
        error("Listen error");
    }

    for (;;) { /* Comenzamos con el bucle infinito */
        
        /* Pedimos memoria para el socket */
        soclient = malloc(sizeof(int));

        /* Aceptamos las conexiones */
        clientelen = sizeof(clientedir);
        if ((*soclient = accept(sock
                              , (struct sockaddr *) &clientedir
                              , &clientelen)) == -1) {
            exit_protocol();
            error("No se puedo aceptar la conexión. ");
        }
        
        printf("Acepté una conexión. ");

        /* Creamos el thread que maneja este cliente y le pasamos como argumneto su socket */
        pthread_create(&thread , NULL , rutina, (void *) soclient);
    }

    /* Código muerto */
    close(sock);
    tablahash_destruir(diccionario);
    return 0;
}


void *rutina(void *_arg){
    int *soclient = (int*)_arg;
    char *nickname;
    int nbytes;

    /* Pasos previos al inicio de rutina */
    nickname = malloc(sizeof(char)*MAX_NICK);
    nickname[0]='\0';

    /* Recibo el nickname del cliente */
    recv(*soclient, nickname, MAXMSG, 0);

    int band_nick_usado = 1;

    /* check_clave_usage busca el nickname en el diccionario, si no está usado lo inserta y retorna 0,
    si esta usado retorna el socket del cliente con dicho nickname */
    while(band_nick_usado) {
        /* Si el nickname está disponible, agrego el cliente a nuestro diccionario */
        pthread_mutex_lock(&lock);
        band_nick_usado = check_clave_usage(diccionario, nickname, soclient);
        pthread_mutex_unlock(&lock);

        if (!band_nick_usado) {
            send(*soclient, "Tu nickname es válido y nos gusta mucho :)\n>Hola! Bienvenidx a la sala de chat", 
            sizeof("Tu nickname es válido y nos gusta mucho :)\n>Hola! Bienvenidx a la sala de chat"), 0);
        } else {
            send(*soclient, "Nickname ya en uso. Prueba con uno nuevo", 
            sizeof("Nickname ya en uso. Prueba con uno nuevo"), 0);

            nickname[0]='\0';
            /* Recibo el nuevo nickname */
            recv(*soclient, nickname, MAXMSG, 0);
        }
    }

    printf("%s entró a la sala\n", nickname);

    int socket = *soclient;
    char *nombre_cliente = nickname;
    char buf[1024];

    /* Recibimos el comando que envía el cliente en buf, parseamos el string para
    interpretar el comando (y los argumentos en caso de que tenga) y realizar lo 
    que corresponda dependiendo del mismo */
    while(recv(socket, buf, sizeof(buf),0) != 0) {
        // buf viene con un /n al final
        char *comando = malloc(sizeof(char)*MAX_COMANDO);
        comando[0] = '\0';
        sscanf(buf, "%s ", comando);
        printf("%s introdujo el comando: %s\n", nombre_cliente, comando);

        if (strcmp(comando, "/msg") == 0) {
            char *nick_reciever = malloc(sizeof(char)*MAX_NICK);
            nick_reciever[0] = '\0'; 
            sscanf(buf+strlen(comando)+1, "%s ", nick_reciever);
            
            pthread_mutex_lock(&lock);
            void *rdo_buscar = tablahash_buscar(diccionario, nick_reciever, 0);
            pthread_mutex_unlock(&lock);
            
            if (rdo_buscar) {
                /* Efectivamente existe un cliente con ese nombre, le mando el mensaje */
                int send_sock = *((int *) rdo_buscar);
                char *message = malloc(sizeof(char)*MAXMSG);
                sscanf(buf+strlen(comando)+strlen(nick_reciever)+2, "%[^\n]\n", message);
                char *msg_chat = malloc(sizeof(char)*(strlen(nombre_cliente)+4+strlen(message)));
                sprintf(msg_chat,"[%s]: %s", nombre_cliente, message);
                msg_chat[strlen(msg_chat)] = '\0';
                nbytes = send(send_sock, msg_chat, strlen(msg_chat)+1, 0);
                if (nbytes < 0) 
                    send(socket, "No se pudo mandar tu mensaje.", sizeof("No se pudo mandar tu mensaje."), 0);
                free(message);
                free(msg_chat);
            } else {
                /* No encontré el cliente nickname en el diccionario */
                printf("No encontré el cliente con nombre %s en el diccionario\n", nick_reciever);
                send(socket, "No está en la sala de chat el usuario especificado", 
                sizeof("No está en la sala de chat el usuario especificado"), 0);
            }
            free(nick_reciever);
        }

        else if (!strcmp(comando, "/nickname")) {
            char *new_nickname = malloc(sizeof(char)*MAX_NICK);
            sscanf(buf+strlen(comando), "%s\n", new_nickname);
            /* Chequea que el nuevo nickname no esté usado, y en 
            ese caso agrega el nuevo y elimina el anterior */
            pthread_mutex_lock(&lock);
            int band_nickname = check_clave_usage(diccionario, new_nickname, soclient);
            if (!band_nickname)
                tablahash_eliminar(diccionario, nombre_cliente);
            pthread_mutex_unlock(&lock);
            if (!band_nickname) {
                nombre_cliente = new_nickname;
                send(socket, "Nickname cambiado!", sizeof("Nickname cambiado!"), 0);
            } else if (band_nickname == socket) {
                send(socket, "Ese ya era tu nickname!",
                sizeof("Ese ya era tu nickname!"), 0);
            } else {
                send(socket, "No fue posible cambiar su nickname: nickname ya en uso",
                sizeof("No fue posible cambiar su nickname: nickname ya en uso"), 0);
            }
        }

        else if (!strcmp(comando, "/exit")) {
            /* Elimino al cliente del chat y el hilo retorna */
            pthread_mutex_lock(&lock);
            tablahash_eliminar(diccionario, nombre_cliente);
            pthread_mutex_unlock(&lock);
            close(socket);
            return NULL;
        }

        else if (!strcmp(comando, "/list")) {
            char *list_users = malloc(sizeof(char)*((MAX_NICK+2)*diccionario->numElems - 1 + 29));
            list_users[0] = '\0';
            sprintf(list_users, "Usuarios en la sala de chat:\n");
            int enter = 0;
            for (unsigned int i = 0; i < diccionario->capacidad; i++) {
                /* Si la casilla en la que estamos fue inicializada y no eliminada */
                if (diccionario->tabla[i].clave && diccionario->tabla[i].estado != 2){
                    if (enter) {
                        sprintf(list_users + strlen(list_users), "\n");
                    }
                    enter = 1;
                    sprintf(list_users+strlen(list_users),">%s", diccionario->tabla[i].clave);
                }
            }
            list_users[strlen(list_users)] = '\0';
            send(socket, list_users, strlen(list_users)+1, 0);
            free(list_users);
        }

        else if (!strcmp(comando, "/all")) {
            /* Le mando el mensaje a todos los clientes */
            char *msg_all = malloc(sizeof(char)*MAXMSG);
            sprintf(msg_all, "[%s]:", nombre_cliente);
            sscanf(buf+strlen(comando), "%[^\n]\n", msg_all+strlen(msg_all));
            msg_all[strlen(msg_all)] = '\0';
            for (unsigned int i = 0; i < diccionario->capacidad; i++) {
                if (diccionario->tabla[i].dato && (diccionario->tabla[i].estado != 2)) {
                    send(*(int *)(diccionario->tabla[i].dato), msg_all, strlen(msg_all)+1, 0);
                }
            }
            free(msg_all);
        }

        else {
            printf("Comando inválido\n");
            send(socket, "El comando ingresado no es válido", 
            sizeof("El comando ingresado no es válido"), 0);
        }
    }

    return NULL;
}
