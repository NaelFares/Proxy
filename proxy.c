#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1" // Définition de l'adresse IP d'écoute
#define SERVPORT "0"         // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1          // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024    // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64        // Taille d'un nom de machine
#define MAXPORTLEN 64        // Taille d'un numéro de port

int main()
{
    int ecode;                      // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];    // Adresse du serveur
    char serverPort[MAXPORTLEN];    // Port du server
    int descSockRDV;                // Descripteur de socket de rendez-vous
    int descSockCOM;                // Descripteur de socket de communication
    struct addrinfo hints;          // Contrôle la fonction getaddrinfo
    struct addrinfo *res;           // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo; // Informations sur la connexion de RDV
    struct sockaddr_storage from;   // Informations sur le client connecté
    socklen_t len;                  // Variable utilisée pour stocker les
                                    // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];      // Tampon de communication entre le client et le serveur
    char login[MAXBUFFERLEN];        // Extraire le login

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1)
    {
        perror("Erreur création socket RDV\n");
        exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;     // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_family = AF_INET;       // seules les adresses IPv4 seront présentées par
                                     // la fonction getaddrinfo

    // Récupération des informations du serveur
    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
    if (ecode)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }
    // Publication de la socket
    ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
    if (ecode == -1)
    {
        perror("Erreur liaison de la socket de RDV");
        exit(3);
    }
    // Nous n'avons plus besoin de cette liste chainée addrinfo
    freeaddrinfo(res);

    // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
    len = sizeof(struct sockaddr_storage);
    ecode = getsockname(descSockRDV, (struct sockaddr *)&myinfo, &len);
    if (ecode == -1)
    {
        perror("SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr *)&myinfo, sizeof(myinfo), serverAddr, MAXHOSTLEN,
                        serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ecode != 0)
    {
        fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
        exit(4);
    }
    printf("L'adresse d'ecoute est: %s\n", serverAddr);
    printf("Le port d'ecoute est: %s\n", serverPort);

    // Definition de la taille du tampon contenant les demandes de connexion
    ecode = listen(descSockRDV, LISTENLEN);
    if (ecode == -1)
    {
        perror("Erreur initialisation buffer d'écoute");
        exit(5);
    }

    len = sizeof(struct sockaddr_storage);
    // Attente connexion du client
    // Lorsque demande de connexion, creation d'une socket de communication avec le client
    while (1) {
        
        descSockCOM = accept(descSockRDV, (struct sockaddr *)&from, &len);
        if (descSockCOM == -1)
        {
            perror("Erreur accept\n");
            exit(6);
        }

        pid_t fils = fork();

        if (fils == -1)
            {
                perror("Erreur fork\n");
                exit(7);
            }

        if (fils == 0) {

            close(descSockRDV); // Fermer la socket de RDV dans le processus enfant

            // Echange de données avec le client connecté

            /*****
                * Testez de mettre 220 devant BLABLABLA ...
                * **/

            // le proxy ecrit au client le message du serveur 220
            strcpy(buffer, "220 Proxy FTP ready, bienvenue dans le proxy NN !\n");
            write(descSockCOM, buffer, strlen(buffer));

            /*******
                *
                * A vous de continuer !
                *
                * *****/

            // Le proxy lis la reponse du client et découpe pour séparer login et adresse
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté client\n");
                exit(3);
            }
            // Ajoute un caractère vide à la fin de la chaîne 
            buffer[ecode] = '\0';
            printf("Message bien reçu ! : \"%s\".\n", buffer);

            // Découpage du login et du serveur dans la chaine envoyée au format ------------->  login@adressServ
            sscanf(buffer, "%[^@]@%s", login, serverAddr);
            printf("Login : %s\n", login);
            printf("Adresse : %s\n", serverAddr);

            int descSockServer;        // Descripteur pour la connexion au serveur
            char *portServer = "21"; // Descripteur pour la connexion au serveur, port 21 pour ftp

            // Création de la Socket pour la connexion au serveur afin que le proxy soit connecté au serveur
            // il utilise le serverAddr du client pour effectuer la connexion avec le serveur
            ecode = connect2Server(serverAddr, portServer, &descSockServer);
            if (ecode == -1) {
                perror("Erreur :Problème de lecture\n");
                exit(1);
            } else {
                printf("Connexion réussie\n");
            }

            // Le proxy lis le message  -------------> (220 Welcome ... ) du serveur
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1) {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            // Ajoute un caractère vide à la fin de la chaîne 
            printf("Message reçu du serveur ! : \"%s\".\n", buffer);
            strcpy(buffer, login);
            // On ajoute le \n et le retour de chariot 
            strcat(buffer,"\r\n");

            //  Envoi du login au serveur par le proxy 
            write(descSockServer, buffer, strlen(buffer));

            //Lecture de la reponse du serveur -------------> (331 login ok...) qui demande le mot de passe après la transmission du login
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1) {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            // Ajoute un caractère vide à la fin de la chaîne 
            buffer[ecode] = '\0';
            printf("Message reçu du serveur ! : \"%s\".\n", buffer);

            // Proxy écrit au client le message reçu par le serveur, demande du mot de passe suite à l'envoi du login (échange sur la socket proxy serveur)
            write(descSockCOM, buffer, strlen(buffer));

            // Lecture du mot de passe du client par le proxy
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            if (ecode == -1) {
                perror("Problème de lecture du côté client\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message reçu du client ! : \"%s\".\n", buffer);

            // Ecrit le mot de passe sur au serveur  -------------> (PASS XXX)
            write(descSockServer, buffer, strlen(buffer));

            // Lecture de la réponse du serveur confirme la connexion -------------> (230 Anonymous access granted)
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';

            printf("Message reçu du serveur ! : \"%s\".\n", buffer);

            // Renvoi le message de confirmation de connexion 230 ...
            write(descSockCOM, buffer, strlen(buffer));    

            // Lecture de la demande SYST du client par le proxy
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté client\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message reçu du client ! : \"%s\".\n", buffer);

            // Envoi la demande SYST au serveur
            write(descSockServer, buffer, strlen(buffer));   

            // Lecture de la réponse du serveur -------------> (215 UNIX Type: L8)
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message reçu du serveur ! : \"%s\".\n", buffer);

            // Envoi des réponses de SYST au client
            write(descSockCOM, buffer, strlen(buffer));

            // Reception du port du client -------------> (PORT 127.....)
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message reçu du client ! : \"%s\".\n", buffer);

            //Découpage pour récupérer les informations de connexion (IP et port)
            int n1, n2, n3, n4, n5, n6; // Aréation de toute les partie à decouper du port
            sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6); // Associée chaque partie du port aux variables
            char adrrClient[MAXHOSTLEN];
            char portClient[MAXPORTLEN];
            sprintf(adrrClient, "%d.%d.%d.%d", n1, n2, n3, n4);
            sprintf(portClient, "%d", n5 * 256 + n6); // calcul du numéro de port : 205*256 + 179 = 52659

            int sockDataClient; //Nouveau client pour connexion a la socket pour le mode passif                

            // Création du socket pour transférer les données (liste) entre le proxy et le client
            ecode = connect2Server (adrrClient, portClient, &sockDataClient);
            if (ecode == -1) {
                perror("Erreur :Problème de lecture du côté client\n");
            }else {
                printf("Connexion sockDataClient réussie\n" );
            }

            // Envoyer PASV au serveur
            strcpy(buffer, "PASV\r\n"); 

            printf("Envoi de pasv au serveur à partir du code : \"%s\".\n", buffer);
            // Mode passif envoyé au serveur
            write(descSockServer, buffer, strlen(buffer));

            // Lecture du message de confirmation du serveur, -------------> 227 Entering passive mode (10,2,0,7,8,41) 
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message reçu du serveur ! : \"%s\".\n", buffer);

            // Découpage du message pour récupérer les informations de connexion (IP et port)
            n1, n2, n3, n4, n5, n6 = 0;
            // On esquive le contenu avant la première parenthèse
            sscanf(buffer, "%*[^(](%d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);
            char adresseServeur[MAXHOSTLEN];
            char portServeur[MAXPORTLEN];
            sprintf(adresseServeur, "%d.%d.%d.%d", n1, n2, n3, n4);
            sprintf(portServeur, "%d", n5 * 256 + n6);

            int sockDataServer;

            // Création d'un socket pour le transfert des données (liste) entre le proxy et le serveur
            ecode = connect2Server (adresseServeur, portServeur, &sockDataServer);
            if (ecode == -1) {
                perror("Erreur : Problème de lecture du côté serveur\n");
            }else {
                printf("Connexion sockDataServer réussie\n" );
            }

            // Informer le client que la connexion est établie
            strcpy(buffer, "220 PORT Commande Réussie !\r\n");

            printf("Message pour le client : \"%s\".\n", buffer);
            // Envoi du message lu au client  
            write(descSockCOM, buffer, strlen(buffer));

            // Lecture de la demande LIST du client  
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message reçu du client ! : \"%s\".\n", buffer);

            // Envoi de la demande LIST au serveur
            write(descSockServer, buffer, strlen(buffer));
            printf("Envoi LIST au serveur\n");

            // Lecture du message envoyé par le serveur -------------> (150 Opening ASCII mode...)
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Erreur lecture dans socket\n");
                exit(9);
            }
            buffer[ecode] = '\0';
            printf("Message recu du serveur : %s\n", buffer);

            // Envoi du message au client
            write(descSockCOM, buffer, strlen(buffer));

            // Récupère la première ligne de la liste retournée par le serveur et la stocke dans le socket serveur
            ecode = read(sockDataServer, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Problème de lecture du côté serveur\n");
                exit(3);
            }
            buffer[ecode] = '\0';
            printf("Message bien reçu du server socket ! : \"%s\".\n", buffer);

            // Récupère le reste de la liste
            while (ecode != 0)
            {
                ecode = write(sockDataClient, buffer, strlen(buffer)); // Stockage de la liste dans le socket client
                if (ecode == -1)
                {
                    perror("Erreur écriture dans socket\n");
                    exit(10);
                }
                bzero(buffer, MAXBUFFERLEN );   // Efface le contenu du buffer
                ecode = read(sockDataServer, buffer, MAXBUFFERLEN ); // Récupère le reste de la liste
                if (ecode == -1)
                {
                    perror("Erreur lecture dans socket\n");
                    exit(9);
                }
            }

            close(sockDataClient); // Fermeture du socket entre le client et le proxy
            close(sockDataServer); // Fermeture du socket entre le proxy et le serveur

            // Lecture du message de réussite par le serveur -------------> (226 Transfer complete)
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode == -1)
            {
                perror("Erreur lecture dans socket\n");
                exit(9);
            }
            buffer[ecode] = '\0';
            printf("Message recu du serveur : %s\n", buffer);

            // Envoi de la liste au client
            ecode = write(descSockCOM, buffer, strlen(buffer));
            if (ecode == -1)
            {
                perror("Erreur écriture dans socket\n");
                exit(10);
            }
            bzero(buffer, MAXBUFFERLEN); // Efface le contenu du buffer

            // Fermeture de la connexion
            printf("Fermeture de la connexion\n");

            // Fermeture de la connexion
            close(descSockServer);
            // Fermer la connexion dans le processus enfant
            close(descSockCOM);
            exit(0);
        }

        // Processus parent
        close(descSockCOM); // Fermer la socket de communication dans le processus parent
    }

    return 0;
}
