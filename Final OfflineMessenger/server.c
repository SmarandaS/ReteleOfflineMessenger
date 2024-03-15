#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

int callback(void *, int, char **, char **);

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData
{
    int idThread; // id-ul thread-ului tinut in evidenta de acest program
    int cl;       // descriptorul intors de accept
    
    int idUtil;   //id Utilizator curent

} thData;

thData *utilizatori[50];
int nrUtil = 0;

static void *treat(void *); /*functie executata de fiecare thread ce realizeaza comunicarea cu clientii*/

int raspunde(void *);

int main()
{

    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("conturi.db", &db);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;
    int nr; // mesajul primit de trimis la client
    int sd; // descriptorul de socket
    int pid;
    pthread_t th[100]; // Identificatorii thread-urilor care se vor crea
    int i = 0;
    // nrUtil=9;

    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
    /* utilizarea optiunii SO_REUSEADDR */
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }
    /* servim in mod concurent clientii...folosind thread-uri */
    while (1)
    {
        int client;
        thData *td; // parametru functia executata de thread
        int length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        /* s-a realizat conexiunea, se astepta mesajul */

        // int idThread; //id-ul threadului
        // int cl; //descriptorul intors de accept

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i;
        td->cl = client;
        td->idUtil = -1; // id ul utiliz= id thread iar id urile threadurilor vor incepe de la o
        utilizatori[i] = td;
        nrUtil = i;
        i++;
        pthread_create(&th[i], NULL, &treat, td);

    } // while
};

static void *treat(void *arg) /*functie executata de fiecare thread ce realizeaza comunicarea cu clientii*/
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    raspunde((struct thData *)arg);
    /* am terminat cu acest client, inchidem conexiunea */
    close((intptr_t)arg);
    return (NULL);
};

struct utiliz
{
    int ID;
    char Nume[20];
    int online;
};

struct tab_mesaje
{
    int sursa;
    int dest;
    char mesaj[100];
    int trimis;
};

int callback(void *verificare, int argc, char **argv, char **azColName)
{
    // NotUsed = 0;
    // char *pExista = (char*)verificare;
    ((struct utiliz *)verificare)->ID = atoi(argv[0]);
    strcpy(((struct utiliz *)verificare)->Nume, argv[1]);
    // strcpy(pExista,argv[0]);
    /*for (int i = 0; i < argc; i++)
    {
        printf("%s\n", argv[i] ? argv[i] : "NULL");
    }
    printf("\n");*/
    return 0;
}

int callbackOnline(void *verificare, int argc, char **argv, char **azColName)
{

    ((struct utiliz *)verificare)->online = atoi(argv[0]);
    return 0;
}

int callbackMess(void *verificare, int argc, char **argv, char **azColName)
{
    //((struct tab_mesaje *)verificare)->sursa = atoi(argv[0]);
    ((struct tab_mesaje *)verificare)->sursa = atoi(argv[0]);
    strcpy(((struct tab_mesaje *)verificare)->mesaj, argv[1]);
    return 0;
}

int regist(int sd, thData *ptd)
{
    // int ID = nrUtil;
    char *nume = (char *)malloc(40 * sizeof(char)); // imi da "Eroare la citirea din client. Bad address" daca nu pun asa
    char *confirmare = (char *)malloc(100 * sizeof(char));
    int status = 0;
    sqlite3 *db = NULL;
    char *err_msg = 0;
    bzero(nume, sizeof(nume));
    bzero(confirmare, sizeof(confirmare));

    if (read(ptd->cl, nume, sizeof(nume)) <= 0)
    {
        printf("[Thread %d]\n", ptd->idThread);
        perror("Eroare la citirea de la client.\n");
    }

    int rc = sqlite3_open("conturi.db", &db);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    char stmt[100];

    sprintf(stmt, "INSERT INTO Users (Name, Online)"
                  "VALUES ('%s', '%d')",
            nume, status);

    rc = sqlite3_exec(db, stmt, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    strcat(confirmare, "Inregistrat");
    if (write(ptd->cl, confirmare, sizeof(confirmare)) <= 0)
    {
        printf("[Thread %d] ", ptd->idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    }
    else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", ptd->idThread);

    if (db != NULL)
        sqlite3_close(db);

    return 0;
}

// char exista[200];
char verif[200];

int login(int sd, thData *ptd)
{
    // int ID = nrUtil;
    char *nume = (char *)malloc(40 * sizeof(char)); // imi da "Eroare la citirea din client. Bad address" daca nu pun asa
    char *confirmare = (char *)malloc(100 * sizeof(char));
    int status = 1; // cand se logheaza clientul statusul lui devine 1
    sqlite3 *db = NULL;
    char *err_msg = 0;
    char exista[200];
    struct utiliz ex;
    ex.ID = -1;
    // char *exista = 0;
    bzero(exista, sizeof(exista));
    bzero(verif, sizeof(verif));
    bzero(nume, sizeof(nume));
    bzero(confirmare, sizeof(confirmare));

    if (read(ptd->cl, nume, sizeof(nume)) <= 0)
    {
        printf("[Thread %d]\n", ptd->idThread);
        perror("Eroare la citirea de la client.\n");
    }

    int rc = sqlite3_open("conturi.db", &db);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    char verif[100];

    sprintf(verif, "SELECT Id, Name FROM Users WHERE Name = '%s'", nume);

    exista[0] = '\0';

    rc = sqlite3_exec(db, verif, callback, &ex, &err_msg);

    /*printf("Ajuns aici\n");

    printf("%s" , exista);
    printf("\n");

    printf("si aici\n");*/

    printf("%d", ex.ID);
    printf("\n");
    printf("%s", nume);
    printf("\n");

    if (ex.ID > -1)
    {
        printf("Exista\n");
        ptd->idUtil = ex.ID;
    }

    else
        printf("Nu exista\n");

    // printf("%d", ptd->idUtil);
    printf("\n");
    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    char stmt[100];
    sprintf(stmt, "UPDATE Users SET Online = '%d'"
                  "WHERE Name = '%s'",
            status, nume);

    rc = sqlite3_exec(db, stmt, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    if (ex.ID > -1)
    {
        strcat(confirmare, "Logat");
    }

    else
        strcat(confirmare, "Nelogat");

    if (write(ptd->cl, confirmare, sizeof(confirmare)) <= 0)
    {
        printf("[Thread %d] ", ptd->idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    }
    else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", ptd->idThread);

    if (db != NULL)
        sqlite3_close(db);

    return 0;
}

int logout(int sd, thData *ptd)
{

    if (ptd->idUtil == -1)
    {
        printf("Nu sunteti logat!\n");
        return 0;
    }

    // int ID = nrUtil;
    char *nume = (char *)malloc(40 * sizeof(char)); // imi da "Eroare la citirea din client. Bad address" daca nu pun asa
    char *confirmare = (char *)malloc(100 * sizeof(char));
    int status = 0; // cand se delogheaza clientul statusul lui devine 0
    sqlite3 *db = NULL;
    char *err_msg = 0;
    char exista[200];
    struct utiliz ex;
    ex.ID = -1;

    bzero(nume, sizeof(nume));
    bzero(exista, sizeof(exista));
    bzero(confirmare, sizeof(confirmare));

    /*if (read(ptd->cl, nume, sizeof(nume)) <= 0)
    {
        printf("[Thread %d]\n", ptd->idThread);
        perror("Eroare la citirea de la client.\n");
    }
*/
    int rc = sqlite3_open("conturi.db", &db);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    char verif[100];

    sprintf(verif, "SELECT Online FROM Users WHERE Id = '%d'", ptd->idUtil);

    exista[0] = '\0';

    rc = sqlite3_exec(db, verif, callbackOnline, &ex, &err_msg);

    /*printf("Ajuns aici\n");

    printf("%s" , exista);
    printf("\n");

    printf("si aici\n");*/

    printf("%d", ex.ID);
    printf("\n");
    // printf("%s", nume);
    printf("\n");

    if (ex.online > 0)
    {
        printf("Utilizatorul va fi delogat\n");
    }

    else
        printf("Utilizatorul nu este conectat\n");

   
    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    char stmt[100];
    sprintf(stmt, "UPDATE Users SET Online = '%d'"
                  "WHERE Id = '%d'",
            status, ptd->idUtil);

    rc = sqlite3_exec(db, stmt, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    ptd->idUtil = -1;

    

    if (db != NULL)
        sqlite3_close(db);

    return 0;
}

int trimite(int sd, thData *ptd)
{

    if (ptd->idUtil == -1)
    {
        printf("Nu sunteti logat!\n");
        return 0;
    }

    // int ID = nrUtil;
    char *nume = (char *)malloc(40 * sizeof(char)); // imi da "Eroare la citirea din client. Bad address" daca nu pun asa
    char *confirmare = (char *)malloc(100 * sizeof(char));
    int status = 1; // cand se logheaza clientul statusul lui devine 1
    sqlite3 *db = NULL;
    char *err_msg = 0;
    char exista[200];
    struct utiliz ex;
    ex.ID = -1;
    char mesaj[100];
    

    bzero(exista, sizeof(exista));
    bzero(verif, sizeof(verif));
    bzero(nume, sizeof(nume));
    bzero(mesaj, sizeof(mesaj));
    bzero(confirmare, sizeof(confirmare));

    printf("urmeaza sa citesc util\n");

    if (read(ptd->cl, nume, sizeof(nume)) <= 0)
    {
        printf("[Thread %d]\n", ptd->idThread);
        perror("Eroare la citirea de la client.\n");
    }

    strcat(confirmare, "Primit d");
    if (write(ptd->cl, confirmare, sizeof(confirmare)) <= 0)
    {
        printf("[Thread %d] ", ptd->idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    }
    else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", ptd->idThread);

    printf("urmeaza sa citesc mesaj\n");

    if (read(ptd->cl, mesaj, sizeof(mesaj)) <= 0)
    {
        printf("[Thread %d]\n", ptd->idThread);
        perror("Eroare la citirea de la client.\n");
    }

    printf("%s", mesaj);
    printf("\n");

    bzero(confirmare, sizeof(confirmare));
    strcat(confirmare, "Primit m");
    if (write(ptd->cl, confirmare, sizeof(confirmare)) <= 0)
    {
        printf("[Thread %d] ", ptd->idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    }
    else
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", ptd->idThread);
    int rc = sqlite3_open("conturi.db", &db);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    char verif[100];

    sprintf(verif, "SELECT Id, Name FROM Users WHERE Name = '%s'", nume);

    exista[0] = '\0';

    rc = sqlite3_exec(db, verif, callback, &ex, &err_msg);

    /*printf("Ajuns aici\n");

    printf("%s" , exista);
    printf("\n");

    printf("si aici\n");*/

    printf("%d", ex.ID);
    printf("\n");
    printf("%s", nume);
    printf("\n");
    int trimis = 0;
    if (ex.ID > -1)
    {
        printf("Exista destinatarul\n");

        char stmt[1000];

        sprintf(stmt, "INSERT INTO Mesaje (Sursa, Destinatie, Mesaj, Trimis)"
                      "VALUES ('%d', '%d', '%s', '%d')",
                ptd->idUtil, ex.ID, mesaj, trimis);

        rc = sqlite3_exec(db, stmt, 0, 0, &err_msg);
    }

    else
        printf("Nu exista destinatarul\n");

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    if (db != NULL)
        sqlite3_close(db);

    return 0;
}

int cere(int sd, thData *ptd)
{

    if (ptd->idUtil == -1)
    {
        printf("Nu sunteti logat!\n");
        return 0;
    }

    printf("am intrat in cere mesaj\n");
    // int ID = nrUtil;
    char *nume = (char *)malloc(40 * sizeof(char)); // imi da "Eroare la citirea din client. Bad address" daca nu pun asa
    char *confirmare = (char *)malloc(100 * sizeof(char));
    int status = 1; // cand se logheaza clientul statusul lui devine 1
    sqlite3 *db = NULL;
    char *err_msg = 0;
    char exista[200];
    struct utiliz ex;
    struct tab_mesaje cer;
    cer.sursa = -1;

    ex.ID = -1;
    char mesaj[100];
    int sent = 0;

    bzero(exista, sizeof(exista));
    bzero(verif, sizeof(verif));
    bzero(nume, sizeof(nume));
    bzero(mesaj, sizeof(mesaj));
    bzero(confirmare, sizeof(confirmare));

    printf("urmeaza sa citesc util\n");

    

    int rc = sqlite3_open("conturi.db", &db);

    if (rc != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    

    char stmt[1000];
    char existamess[100];

    sprintf(existamess, "SELECT Sursa, Mesaj FROM Mesaje WHERE Destinatie = '%d' AND Trimis = '%d'", ptd->idUtil, sent);

    exista[0] = '\0';

    rc = sqlite3_exec(db, existamess, callbackMess, &cer, &err_msg);

    if (rc != SQLITE_OK)
        {

            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);

            return 1;
        }

    printf("%s" , ex.Nume);

    if (cer.sursa != -1)
    {
        char trimmesaj[1000];
        printf("Avem mesaje de la %d\n", cer.sursa);
        printf("Mesaje: %s\n", cer.mesaj);

        char verif[100];

        sprintf(verif, "SELECT Id, Name FROM Users WHERE Id = '%d'", cer.sursa);

        exista[0] = '\0';

        rc = sqlite3_exec(db, verif, callback, &ex, &err_msg);

        sprintf(trimmesaj, "Sursa: %s  Mesaj: %s", ex.Nume, cer.mesaj);

        sent = 1;

        write(sd, trimmesaj, sizeof(trimmesaj));

        

        char stmt[100];

        sprintf(stmt, "UPDATE Mesaje SET Trimis = '%d'"
                      "WHERE Sursa = '%d' AND Destinatie = '%d'",
                sent, cer.sursa, ptd->idUtil);

        rc = sqlite3_exec(db, stmt, 0, 0, &err_msg);

        if (rc != SQLITE_OK)
        {

            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);

            return 1;
        }

        printf("\n");
    }

   

    if (db != NULL)
        sqlite3_close(db);

    return 0;
}

int raspunde(void *arg)
{

    int nr, i = 0;
    char comanda[100];
    char raspuns[50];
    struct thData tdL;
    tdL = *((struct thData *)arg);
    while (1)
    {

        if (read(tdL.cl, comanda, sizeof(comanda)) <= 0)
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la citirea de la client.\n");
        }

        if (strstr(comanda, "register") != 0)
        {
            nrUtil++;
            regist(tdL.cl, (struct thData *)arg);
        }

        if (strstr(comanda, "login") != 0)
        {

            login(tdL.cl, (struct thData *)arg);
        }

        if (strstr(comanda, "logout") != 0)
        {

            logout(tdL.cl, (struct thData *)arg);
        }

        if (strstr(comanda, "quit") != 0)
        {

            printf("Se va inchide sesiunea, nu mai puteti da comenzi\n");
            exit(1);
        }

        if (strstr(comanda, "send") != 0)
        {
            // printf("Am primit send\n");

            trimite(tdL.cl, (struct thData *)arg);
        }

        if (strstr(comanda, "fetch") != 0)
        {
            // printf("Am primit send\n");

            cere(tdL.cl, (struct thData *)arg);
        }
    }
}
