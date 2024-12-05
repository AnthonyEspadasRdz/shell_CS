#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 9999

void main(int argc, char *argv[])
{
  struct sockaddr_in socket_cliente;
  struct hostent *h;
  int fd;
  int n;
  char *host;
  char buf_ser[512];
  char buf_cli[512];

  // Genera el File Descriptor que usará el cliente durante la conexión
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // Inicializa los atributos para establecer conexión con el servidor (struct sockaddr_in)
  memset((char *) &socket_cliente, 0, sizeof(socket_cliente));
  socket_cliente.sin_family = AF_INET;
  socket_cliente.sin_port = htons((u_short) PORT);
  h = gethostbyname( argv[1] );
  memcpy(&socket_cliente.sin_addr, h->h_addr, h->h_length);

  // Envía la solicitud de conexiń
  connect(fd, (struct sockaddr *) &socket_cliente, sizeof(socket_cliente));

  // Espera a recibir respuesta del servidor
  while (1)
  {
    //---printf("\tEmpieza el while\n");

    n = recv(fd, buf_ser, sizeof(buf_ser), 0);
    buf_ser[n] = (char*)0;

    // Si se entro por llamada de exec, muestra entrada pero no da respuesta
    if (!strcmp(buf_ser, "exec")){
      //---printf("\tRecibe exec\n");
      
      memset(&(buf_ser), '\0', 512); 
      n = recv(fd, buf_ser, sizeof(buf_ser), 0);
      //---printf("\trecv devolvio %i\n", n);
      
      buf_ser[n] = (char*)0;
      write(1, buf_ser, n);
      //---printf("\tVa el continue\n\n");
      continue;
    }
    
    // Verifica que no se haya indicado la salida desde el servidor
    if (!strcmp(buf_ser, "exit")){  
      printf("\n");
      break;
    }

    // De no ser el caso, muestra la respuesta del servidor
    //---printf("\tMostramos mensaje\n");
    write(1, buf_ser, n);

    //---printf("\n\tEspera peticion\n");
    // Lee la respuesta del usuario, se elimina el salto de linea
    
    n = read(0, buf_cli, sizeof(buf_cli));
    buf_cli[n-1] = (char*)0;
    buf_cli[n] = 0;
        
    //---printf("\tLee peticion\n");

    // Envía mensaje al servidor tras la conexión
    send(fd, buf_cli, sizeof(buf_cli), 0);
    //---printf("\tSe envia: \"%s\"\n", buf_cli);
    
    // Reinicimos el valor en los buffers
    memset(&(buf_ser), '\0', 512);
    memset(&(buf_cli), '\0', 512);
  }

    // Finaliza la conexión, tras el envío del mensaje, cerrando el File Descriptor
    close(fd);
    exit(0);
}