#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


#define CLIENTES 10
#define PORT 9999

int identificarSalida(char *mensaje);                               // Funcion que determina cuando se debe salir del programa 
int determinarCaso(char *mensaje);                                  // Funcion que analiza la entrada del usuario
char **desglosarComandos(char *mensaje);                            // Funcion que separa los comandos individuales
char **desglosarPipe(char *mensaje);                                // Funcion que separa los comandos divididos por un pipe

int main()
{
    // Variables requeridas para levantar el servidor  
    struct sockaddr_in servidor;
    struct sockaddr_in cliente;
    struct hostent* info_cliente;
    int fd_s, fd_c, n;
    int longClient;
    char buf[256];

    // Variables usadas para la ejecución de comandos 
    char *arg[10];                                                  // Arreglo donde se colocan los comandos individuales
    char *comandoPipe1[10];                                         // Arreglo para los comandos antes del pipe
    char *comandoPipe2[10];                                         // Arreglo para los comandos después del pipe
    int caso;                                                       // Variable que determina como se ejecutaran los comandos
    int sigue = 1;                                                  // Variable para controlar el loop while
    
    // Generamos el File Descriptor para el servidor
    fd_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	      
    // Se inicializan los valores del servidor (struct sockaddr_in)
    memset((char *) &servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((u_short) PORT);
    memset(&(servidor.sin_zero), '\0', 8);

    // Asocia el descriptor de archivo del servidor con su estructura correspondiente
    bind(fd_s, (struct sockaddr *) &servidor, sizeof(servidor));

    // Espera solicitud de conexión
    printf("Esperando conexión...\n");
    listen(fd_s, CLIENTES);

    longClient = sizeof(cliente);

    // Establece conexión y genera File Descriptor para el cliente
    fd_c = accept(fd_s, (struct sockaddr *) &cliente, &longClient);

    // Obtiene la información del cliente y muestra desde donde se realiza la conexión
    info_cliente = gethostbyaddr((char *) &cliente.sin_addr, sizeof(struct in_addr), AF_INET);
    printf("Conectado desde: %s\n\n", info_cliente -> h_name);

// ------------------------------------ Ejecucion de comandos

    // Coloca la conexion como salida estandar

    while (sigue != 0)
    {
        //printf("Introduce los comandos a ejecutar:\n");
        send(fd_c, "\nServidor_AC$ ", 14, 0);
        
        do{
        // Utiliza la variable n para detectar si ha recibido peticiones
        memset(&(buf), '\0', 256);
        n = recv(fd_c, buf, sizeof(buf), 0);
        
        // Colocamos NULL al final del buffer
        buf[n] = (char*)0;
        //---printf("Se recibió \"%i\"\n", *buf); // Valor ASCII
        //---printf("Se recibió \"%s\"\n", buf); // Cadena en el buffer
        }
        // Se omiten las respuestas que no tinen argumentos
        while (*buf < 1);

        // Identifica si hay 1 o mas comandos
        caso = determinarCaso(buf);

        // Se valida que el primer comando sea distinto de 'exit'
        sigue = identificarSalida(buf);
        
        // Cuandos se identifica un 'exit' se termina la ejecucion y notifica al cliente
        if (!sigue){  
            send(fd_c, "exit", 5, 0);
            break;}                                             

        // Se crea un hijo para ejecutar los comandos sin pipe
        if (caso == 0)                                          
        {
            // Obtiene los apuntadores a los strings separados de cada comando
            char **apuntadores = desglosarComandos(buf);
            
            // Copia los apuntadores al arreglo de apuntadores de la función main
            int i;                                              
            for (i = 0; apuntadores[i] != NULL; i++)            
            {
                arg[i] = apuntadores[i];                        
            }

            // Añade el NULL final al arreglo
            arg[i] = apuntadores[i];

            send(fd_c, "exec", 5, 0);                       // Notifica que el mensaje es exec

            int pid = fork();                                   // Creamos el proceso hijo que se encargará de la ejecución
            if (pid == 0)                                           // Validamos que sea el proceso hijo
            {   
                //---printf("\tFork hace exec\n");
                dup2(fd_c, STDOUT_FILENO);                      // Colocamos el FD del cliente como la salida estándar
                execvp(arg[0], arg);                            // Ejecuta los comandos
                exit(1);                                        // De no presentar comandos válidos, termina 
            } else                                              // Instrucciones para el proceso padre
            {
                //---printf("\tPadre pasa a WAIT\n");
                wait(NULL);                                     // Espera que el proceso hijo finalice
            }
        }

        // Cuando se introdujo con comando con pipe
        else if (caso == 1)                                   
        {
            char **apuntadores = desglosarPipe(buf);        // Obtiene los apuntadores a los strings separados

            int indice = 0;                                     // Variable de apoyo para recorrer 'apuntadores' en la asignación

            for (int i = 0; apuntadores[i] != "|"; i++)         // Asignamos los valores anteriores al pipe al lado izquierdo
            {
                comandoPipe1[i] = apuntadores[indice];
                indice++;                        
            }

            comandoPipe1[indice] = (char*)0;                    // Agregamos el final al lado izquierdo
            indice++;

            for (int i = 0; apuntadores[i] != NULL; i++)        // Asignamos los valores posteriores al pipe al lado derecho
            {
                comandoPipe2[i] = apuntadores[indice];
                indice++;                        
            }

            comandoPipe2[indice] = (char*)0;                    // Agregamos el final al lado derecho

            int pid = fork();                                   // Creacion del proceso hijo

            if (pid)                                            // El proceso padre espera la finalizacion de los hijos
            {
                wait(NULL);
            } else                                              // El proceso hijo ejecuta los pipes
            {
                int pipefd[2];
                pipe(pipefd);

                pid = fork();                                   // Segundo proceso hijo para ejecucion de comandos pipe
                if (pid)
                {
                    close(0);                                   // Cerramos la entrada de consola
                    close(pipefd[1]);                           // Cerramos el extremo de escritura del pipe
                    dup(pipefd[0]);                             // Extremo de lectura como entrada estandar
                    wait(NULL);                                 // Espera a que el hijo termine de escribir
                    execvp(comandoPipe2[0], comandoPipe2);      // Ejecuta las instrucciones posteriores al pipe
                    exit(1);                                    // Devuelve la ejecucion del programa al proceso padre 
                } else
                {
                    close(1);                                   // Cerramos la salida a consola
                    close(pipefd[0]);                           // Cerramos el extemo de lectura del pipe
                    dup(pipefd[1]);                             // Extremo de escritura como salida estandar
                    execvp(comandoPipe1[0], comandoPipe1);      // Ejecuta las instrucciones anteriores al pipe
                    exit(1);                                    // Termina la ejecucion para instrucciones no validas
                }
            }    
        }

        for (int i = 0; i < 10; i++)                            // Limpiamos el contenido de  los arreglos despues de cada ejecución
        {
            arg[i] = 0;
            comandoPipe1[i] = 0;
            comandoPipe2[i] = 0;
        }

    }

    // Finalizamos la conexión cerrando el File Descriptor del cliente
    close(fd_c);
// ----------------------------------------------------------------------------------------

    // Dejamos de responder solicitudes cerrando el File Descriptor del servidor
    close(fd_s);
    shutdown( fd_s, SHUT_RDWR );
    exit(0);
}

int identificarSalida(char *mensaje)
{
    int contador = 0;                                           // Contador para recorrer el arreglo
    int receptor = 0;                                           // Contador para el numero efectivo de caracteres leídos
    char exit[4];                                               // Arreglo que recibe los caracteres efectivos leídos

    while(contador < 256)                                        // El limite del contador es el tamaño del arreglo
    {
        if (isblank(mensaje[contador]))                         // Si no hay un caracter, el contador avanza
        {
            contador++;
        } else
        {
            exit[receptor] = mensaje[contador];                 // Si es un caracter, se guarda y aumentan los contadores
            contador++;                                         
            receptor++;
        }

        if (receptor == 4)                                      // Cuando el receptor llega a 4 se han leido 4 caracteres
        {
            return strcmp(exit, "exit");                        // Si se ingreso el comando exit, la salida será 0;
        }
    }
}

int determinarCaso(char *mensaje)
{

    if (strchr(mensaje, '|') == NULL)                           // Busca ocurrencias del operador pipe
    {
        return 0;                                               // Caso 0: no se usa pipe
    } else   
    {
        return 1;                                               // Caso 1: se usa pipe
    }
}

char **desglosarComandos(char *mensaje)
{
    char **arg = malloc(10 * sizeof(char*));                    // Arreglo donde se colocan los comandos individuales
    char *comando;                                              // Variable de apoyo para guardar comandos individuales

    int indice = 0;                                             // Indice de apoyo para guardado de comandos individuales
    comando = strtok(mensaje, " ");                             // Separa los comandos por espacios

    do                                                          // Esta seccion nos e ejecuta cuando solo se recibe un comando
    {
        arg[indice] = comando;                                  // Asigna el comando al arreglo instrucciones
        indice++;                                               // Desplazamos a la siguiente posicion
        comando = strtok(NULL, " ");                            // Continua trabajando sobre la misma cadena
    } while (comando != NULL);                                  // Re repite mientras la cadena no llegue a su fin

    arg[indice] = (char*)0;                                     // Se agrega el final al arreglo de instrucciones

    return arg;                                                 // Devolvemos el apuntador a los comandos
}

char **desglosarPipe(char *mensaje)
{
    char **arg = malloc(sizeof(char*) * 10);                    // Arreglo donde se colocan los comandos individuales
    char **lados = malloc(sizeof(char*) * 2);                   // Arreglo que guarda los dos lados del pipe
    char *comando;                                              // Variable de apoyo para guardar comandos individuales

    lados[0] = strtok(mensaje, "|");                            // Asignamos la parte izquierda del pipe
    lados[1] = strtok(NULL, "\0");                              // Asignamos la parte derecha del pipe

    int indice = 0;                                             // Indice de apoyo para guardado de comandos individuales
    comando = strtok(lados[0], " ");                            // Separa los comandos del lado izquierdo por espacios

    do                                                          // Esta seccion nos e ejecuta cuando solo se recibe un comando
    {
        arg[indice] = comando;                                  // Asigna el comando al arreglo instrucciones
        indice++;                                               // Desplazamos a la siguiente posicion
        comando = strtok(NULL, " ");                            // Continua trabajando sobre la misma cadena
    } while (comando != NULL);                                  // Re repite mientras la cadena no llegue a su fin

    arg[indice] = "|";                                          // Se agrega el pipe 
    indice++;

    comando = strtok(lados[1], " ");                            // Separa los comandos del lado derecho por espacios

    do                                                          // Esta seccion nos e ejecuta cuando solo se recibe un comando
    {
        arg[indice] = comando;                                  // Asigna el comando al arreglo instrucciones
        indice++;                                               // Desplazamos a la siguiente posicion
        comando = strtok(NULL, " ");                            // Continua trabajando sobre la misma cadena
    } while (comando != NULL);                                  // Re repite mientras la cadena no llegue a su fin

    arg[indice] = (char*)0;                                     // Se agrega el final al arreglo de instrucciones

    return arg;                                                 // Devolvemos el apuntador a los comandos
}