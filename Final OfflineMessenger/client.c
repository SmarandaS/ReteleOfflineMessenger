#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sqlite3.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;


int Write(int sockD, char* mesaj, char* raspuns, int sizeRaspuns)
{
    if (write(sockD, mesaj, strlen(mesaj)+1) <= 0)
            {
                perror("[client]Eroare la trimitere comanda spre server.\n");
                return errno;
            }

            bzero(raspuns, sizeRaspuns); 

            if (read(sockD, raspuns, sizeRaspuns) <= 0)
            {
                printf("[client]Eroare la citirea din parinte");
                perror("Eroare la read() de la client.\n");
            }

            printf("[client]Mesajul primit este...%s\n\n\n", raspuns);
}

#define MeniuPrincipal "Optiuni pentru utilizatori:\nregister\nlogin\nlogout\nsend\nfetch\nquit\n"
int main(int argc, char *argv[])
{
    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare
    // char msg[100];                          // mesajul trimis
    int nr = 0;
    char comanda[100];
    char raspuns[100];

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons(port);

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    while (1)
    {

        /* citirea mesajului */
        printf("%s", MeniuPrincipal);
        fflush(stdout);
        bzero(comanda, sizeof(comanda));
        read(0, comanda, sizeof(comanda));
        // nr = atoi(buf);
        //  scanf("%d",&nr);

        printf("[client] Am citit comanda: %s\n", comanda);

        /* trimiterea mesajului la server */
        if (write(sd, comanda, sizeof(comanda)) <= 0)
        {
            perror("[client]Eroare la trimitere comanda spre server.\n");
            return errno;
        }

      

        /* citirea raspunsului dat de server
           (apel blocant pana cand serverul raspunde) */

        if (strstr(comanda, "register") != 0)
        {
            printf("Ajuns\n");
            char nume[40];
            printf("introduceti nume utilizator nou: ");
            scanf("%s", nume);

            Write(sd, nume, raspuns, sizeof(raspuns));
        }

        if (strstr(comanda, "login") != 0)
        {
            printf("Ajuns\n");
            char nume[40];
            printf("introduceti nume utilizator: ");
            scanf("%s", nume);

            Write(sd, nume, raspuns, sizeof(raspuns));
        }

        if (strstr(comanda, "logout") != 0)
        {
            printf("Ajuns\n");
            //printf("Am facut logout\n");
            //char nume[40];
            //printf("introduceti nume utilizator: ");
            //scanf("%s", nume);

            //Write(sd, nume, raspuns, sizeof(raspuns));
            
        }

        if (strstr(comanda, "quit") != 0)
        {
            printf("Ajuns\n");
           
            printf("Se va inchide sesiunea, nu mai puteti da comenzi\n");
            exit(1);
        }

        if (strstr(comanda, "send") != 0)
        {
            printf("Ajuns\n");
            char nume[40];
            char mesaj[100];

            printf("introduceti numele destinatarului: ");
            scanf("%s", nume);

            Write(sd, nume, raspuns, sizeof(raspuns));

            
            printf("introduceti mesajul: ");
            
            scanf("%s", mesaj);

            Write(sd, mesaj, raspuns, sizeof(raspuns));
        }

        if (strstr(comanda, "fetch") != 0)
        {
            printf("Ajuns\n");

            char raspuns[200];

            bzero(raspuns, sizeof(raspuns));
            
            read(sd, raspuns, sizeof(raspuns));

            printf("%s", raspuns);
            printf("\n");
            
        }

        
    }
    close(sd);
}