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

#define PORT 9999

int identificarSalida(char *mensaje);                           // Funcion que determina cuando se debe salir del programa 
int determinarCaso(char *mensaje);                              // Funcion que analiza la entrada del usuario
char **desglosarComandos(char *mensaje);                        // Funcion que separa los comandos individuales
char **desglosarPipe(char *mensaje);                            // Funcion que separa los comandos divididos por un pipe

int main()
{
  struct sockaddr_in servidor;
  struct sockaddr_in cliente;
  struct hostent* info_cliente;
  int fd_s, fd_c, n, fd;
  int longClient;
  char buf[256];

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
  listen(fd_s, 1);

  longClient = sizeof(cliente);

  // Establece conexión y genera File Descriptor para el cliente
  fd_c = accept(fd_s, (struct sockaddr *) &cliente, &longClient);

  // Obtiene la información del cliente y muestra desde donde se realiza la conexión
  info_cliente = gethostbyaddr((char *) &cliente.sin_addr, sizeof(struct in_addr), AF_INET);
  printf("Conectado desde: %s\n\n", info_cliente -> h_name);

  // Coloca la conexion como salida estandar
  dup2(STDOUT_FILENO, fd);
  dup2(fd_c, STDOUT_FILENO);
  printf("Introduce los comandos a ejecutar:\n");

  // Utiliza la variable n para detectar si ha recibido peticiones
  n = recv(fd_c, buf, sizeof(buf), 0);
  
  // El ciclo se mantendrá mostrando mensajes mientras estos lleguen
  while (n > 0) {
    dup2(fd, STDOUT_FILENO);
    write(1, buf, n);
    printf("\n\n");
    dup2(fd_c, STDOUT_FILENO);
    printf("Introduce los comandos a ejecutar:\n");
    n = recv(fd_c, buf, sizeof(buf), 0);
  } 

  // Finalizamos la conexión cerrando el File Descriptor del cliente
  close(fd_c);
  close(fd);  
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

    while(contador < 50)                                        // El limite del contador es el tamaño del arreglo
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