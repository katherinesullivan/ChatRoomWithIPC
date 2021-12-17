# Makefile para el ejercicio de Chat IRC de Sistemas Operativos I

# Macros
DICCIONARIO = tablahash.c
CLIENTE = ChatClient.c
SERVIDOR = ChatServer.c
FLAGS = -Wall -Wextra -Werror


all : cliente servidor clean

# Ejecutable del cliente.
cliente : $(CLIENTE)
	$(CC) $(FLAGS) -o $@ $^

# Ejecutable del servidor.
servidor : $(SERVIDOR) $(DICCIONARIO:.c=.o) 
	$(CC) $(FLAGS) -o $@ $^ -pthread

# Archivo objeto de la implementación de Tablas Hash.
$(DICCIONARIO:.c=.o) : $(DICCIONARIO) $(DICCIONARIO:.c=.h)
	$(CC) $(FLAGS) -c $(DICCIONARIO)


# Destino simbólico que borra todos los archivos con extensión .o cuando se invoca.
clean: 
	rm *.o

.PHONY : clean all cliente servidor
