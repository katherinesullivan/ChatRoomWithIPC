# ChatRoomWithIPC - English

This project implements a chatroom in C using Inter-Process Comunication. Note this repository and all its contents are written in spanish.

------------------------------

# Sala de Chat con IPC - Español

Este proyecto contiene dos archivos principales: [ChatServer.c](ChatServer.c) y [ChatClient.c](ChatClient.c). Su objetivo fue implementar una sala de chat. A continuación se presentan indicaciones para su uso.

## Compilación y ejecución:

1- Para su compilación existe un archivo makefile. Simplemente debe correrse el comando `make` o el comando `make all` y se obtendrán dos archivos ejecutables: `./servidor` y `./cliente`.

2- El primer archivo que debe correrse es `./servidor` (pues sin el servidor en ejecución no vamos a poder ser clientes del mismo) y debe correrse acompañado de un puerto como argumento.

3- Estando el servidor en ejecución podemos empezar la ejecución de los clientes. Acompañando el ejecutable `./cliente` deben ir dos argumentos en el siguiente orden:

+ 1º la dirección Ipv4 donde está siendo ejecutado el servidor

+ 2º el puerto especificado como argumento en el ./servidor

Es decir, al momento de ejecutarlo este tendrá la siguiente forma: `$./cliente DirIp Puerto`.

Tener en cuenta que el servidor soporta escuchar hasta 25 clientes.

## Uso como cliente:

Con el servidor ya corriendo, un cliente primero deberá ingresar su nickname (nombre con el cual aparecerá para el resto de los clientes). Considerar que la máxima longitud para un nickname es de 32 bytes.

En caso de que el nickname sea válido (es decir, ninguno de los otros clintes cuenta con el mismo) se le dará un mensaje de bienvenida. En caso contrario, se le solicitará que ingrese un nickname nuevo.

Con el nickname ya asignado, el cliente pueda hacer uso de 4 comandos. Tener en cuenta que la máxima longitud de un comando (acompañado de sus argumentos) es de 1024 bytes.

1- `/msg` que envía un mensaje privado a algún usuario de la sala. Este toma dos argumentos: `nickname` que especifica el usuario al cual se le desea enviar el mensaje, y `msg` que es el mensaje a enviar. Por ejemplo, si un cliente llamado James desea mandarle un mensaje que diga "Hola, ¿cómo estás?" al cliente con nombre Pepe debería ingresar lo siguiente:

    >/msg Pepe Hola, ¿cómo estás?

2- `/all` que permite enviar un mensaje general a todos los clientes en la sala. Este toma un argumento `msg` que especifica el mensaje general que se desea enviar. Por ejemplo, si un cliente desea enviar un mensaje que diga "¡Hola a todos!" debería ingresar lo siguiente:

    >/all ¡Hola a todos!

3- `/nickname` que le permite al cliente cambiar su nickname. Este recibe un argumento `new_nickname` que especifica cual es el nickname que el cliente pretende tener ahora. Por ejemplo, si un cliente desea cambiar su nickname a Carlota debería ingresar lo siguiente:

    >/nickname Carlota

4- `/list` que permite visualizar todos los nicknames de los usuarios en la sala. Este comando no recibe ningún argumento. Por ejemplo, suponiendo que en la sala de chat se encuentran solamente los clientes mencionados anteriormente, si alguno de ellos ejecutara el comando `/list` en su pantalla aparecería lo siguiente:

    >Usuarios en la sala de chat:
    >Pepe
    >Carlota
    >James

5- `/exit` que le permite a un cliente salir de la sala. Este no recibe ningún argumento y una vez ejecutado termina el programa `./cliente`.

El cliente recibirá distintos mensajes a lo largo de su ejecución. Si estos mensajes vienen precedidos por un nickname entre corchetes significa que son mensajes provenientes del cliente con ese nickname. Si simplemente vienen precedidos por un símbolo `>` y nada más, significa que son mensajes provistos por el servidor. Siguiendo con el ejemplo dado anteriormente, el mensaje que James le mando a Pepe aparecería en la pantalla de Pepe como:

    >[James]: Hola, ¿cómo estás?

Mientras que si Carlota pudo actualizar su nickname le llegará el siguiente mensaje del servidor:

    >Nickname cambiado!.