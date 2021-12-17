#include <sys/socket.h> /* Cabeceras de Sockets */
#include <sys/types.h>
#include <netdb.h> /* Cabecera de direcciones por red */
#include <signal.h> /* Cabecera de Signals */
#include <sys/select.h> /* Cabecera de Select */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


/* El archivo describe un cliente que se conecta al servidor establecido
 * en el archivo ChatServer.c. Se utiliza de la siguiente manera:
 * $./client IP port
*/

#define MAXMSG 1024
int sock;

/* Función que implementa el protocolo de salida */
void exit_protocol(int *socket, char **message) {
    free(*message);
    if (socket) close(*socket);
}

/* Función para salir en caso de error */
void error(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/* Funciones de manejo de señales */
void signal_protocol(int arg) {
    send(sock, "/exit", sizeof("/exit"), 0);
    exit(arg);
}
void child_exit() {
    exit(EXIT_SUCCESS);
}
void parent_exit() {
    exit(EXIT_SUCCESS);
}

/* Función que agrega el caractér terminador de string al final de un string */
void terminate_message(char *message) {
    char c = 'a';
    int i = 0;
    while (c != '\n') { // asumimos que message tiene un \n
        c = message[i];
        i++;
    }
    message[i] = '\0';
}



int main(int argc, char **argv) {
    
    /* Manejo de salida */
    signal(SIGINT, signal_protocol);

    struct addrinfo *resultado;
    ssize_t nbytes;
    char buf[MAXMSG];
    char *message = malloc(sizeof(char) * MAXMSG);

    /* Chequeamos que los argumentos fueron pasados */
    if (argc != 3) {
        fprintf(stderr, "El uso es \'%s IP port\'\n", argv[0]);
        exit_protocol(NULL, &message);
        exit(EXIT_FAILURE);
    }

    /* Inicializamos el socket */
    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        exit_protocol(NULL, &message);
        error("No se pudo iniciar el socket");
    }

    /* Buscamos la dirección del hostname:port y nos conectamos */
    if (getaddrinfo(argv[1], argv[2], NULL, &resultado)){
        exit_protocol(&sock, &message);
        fprintf(stderr,"No se encontro el host: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    if (connect(sock, (struct sockaddr *) resultado->ai_addr, resultado->ai_addrlen) != 0) {
        exit_protocol(&sock, &message);
        error("No se pudo conectar");
    }

    printf("Binding exitoso ^^\n");
    freeaddrinfo(resultado);

    /* Todo armado, comienza la interacción del usuario */
    /* El usuario debe cargar su nickname desde la shell */

    write(1,"nickname: ", sizeof("nickname: "));

    /* Vamos a esperar que llegue algo desde el stdin o del socket*/
    int ret_nfds;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(sock, &readfds);
    ret_nfds = select(sock+1, &readfds, NULL, NULL, NULL);
    if (ret_nfds != -1) {
        if (FD_ISSET(sock, &readfds)) {
            /* Si no hubo un error y llegó un mensaje del servidor 
            (que a esta altura solo puede ser el de terminación)
            se cierra la ejecución del cliente */
            printf("-----Saliendo: servidor cerrado------\n");
            exit_protocol(&sock, &message);
            _exit(0);
        }
    }
    else {
        /* Hubo un error en el select */
        exit_protocol(&sock, &message);
        error("Falló el select");
    }

    /* Si no sucedió lo anterior continuamos con la lectura del nickname */
    if (read(0, (void *)message, MAXMSG) == -1) {
        exit_protocol(&sock, &message);
        error("Falló la lectura del nickname");
    }

    message[strlen(message)-1] = '\0'; // read lee el \n, lo reemplazo por \0

    /* Primera interacción con el servidor: le envío mi nickname */
    nbytes = send(sock, (void*)message, strlen(message)+1, 0);
    if (nbytes < 0) {
        exit_protocol(&sock, &message);
        error("Falló el envío del nickname");
    }

    /* Recibimos la confirmación de nickname válido */
    if (recv(sock, buf, sizeof(buf), 0) == -1) {
        exit_protocol(&sock, &message);
        error("Falló la recepción del mensaje");
    }
    // printf("llego %s\n",buf);
    while ((strcmp(buf, "Tu nickname es valido y nos gusta mucho :)\n>Hola! Bienvenidx a la sala de chat") != 0)) {
        if (strcmp(buf, "/TERM_PROCESS") == 0) {
            printf("-----Saliendo: servidor cerrado------\n");
            exit_protocol(&sock, &message);
            _exit(0);
        }
        if (strcmp(buf, "Nickname ya en uso. Prueba con uno nuevo") == 0) {
            printf(">%s\n", buf);
            write(1,"nickname: ", sizeof("nickname: "));
            if (read(0, (void *)message, MAXMSG) == -1) {
                error("Falló la lectura del nickname");
                exit_protocol(&sock, &message);
            }
            message[strlen(message)-1] = '\0'; // read lee el \n, lo reemplazo por \0
            /* Envío otro nickname */
            nbytes = send(sock, (void*)message, strlen(message)+1, 0);
            if (nbytes < 0) {
                exit_protocol(&sock, &message);
                error("Falló el envío del nickname");
            }
        }
        if (recv(sock, buf, sizeof(buf), 0) == -1) {
            exit_protocol(&sock, &message);
            error("Falló la recepción del mensaje");
        }
    }
    printf(">%s\n", buf);

    /* Creo un nuevo porceso (Child) */
    int child = fork();
    if (child == 0) { // Proceso Child
        /* Manejo de salida */
        signal(SIGHUP,child_exit);

        while(1) {
            buf[0] = '\0';

            /* Recibimos lo que nos manda el servidor */
            if (recv(sock, buf, sizeof(buf), 0) == -1) {
                exit_protocol(&sock, &message);
                error("Falló la recepción del mensaje");
            }
            
            /* Si es el mensaje de terminación del servidor, activo protocolo de salida */
            if (strcmp(buf, "/TERM_PROCESS") == 0) { 
                printf("------Saliendo: servidor cerrado------\n");
                exit_protocol(&sock, &message);
                _exit(0);
            }
            printf(">%s\n", buf);
        }


    } else if (child != -1) { // Proceso Parent
        /* Manejo de salida */
        signal(SIGCHLD,parent_exit);

        while (1) {
            message[0] = '\0';

            /* Estoy esperando algún comando de la shell */
            if (read(0, (void*)message, MAXMSG) == -1) {
                exit_protocol(&sock, &message);
                error("Falló la lectura del mensaje");
            }
            terminate_message(message);

            /* Mando el comando al servidor */
            nbytes = send(sock, (void*)message, strlen(message)+1,0);
            if (nbytes < 0) {
                exit_protocol(&sock, &message);
                error("Falló el envío del mensaje");
            }
            
            /* Si es un /exit, activo protocolo de salida */
            if (strcmp(message,"/exit\n") == 0) { 
                if (kill(child, SIGHUP)) {
                    printf("Kill no ejecutado. Pruebe otra vez\n");
                }
                else {
                    exit_protocol(&sock, &message);
                    _exit(EXIT_SUCCESS);
                }
            }
        }

    } else {
        exit_protocol(&sock, &message);
        error("Falló el fork");
    }

    /* Código muerto */
    free(message);
    close(sock);
    return 0;
}

