
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <sys/wait.h>

int main(int argc, char **argv)
{
	pid_t pid;
	int pipefd[2];

	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return -1;
	}

	pid = fork();
	if (pid == -1)
	{ // Une erreur s'est produite
		perror("fork");
		return -1;
	}

	if (pid == 0)
	{ // Code exécuté par le processus enfant
		while(1)
		{
			static int fdSocketCAN, i; 
			static int nbytes;
			static struct sockaddr_can addr;
			static struct ifreq ifr;
			static struct can_frame frame;

			printf("(Fils) CAN Attente de la trame\r\n");

			if ((fdSocketCAN = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
				perror("Socket");
				printf("(Fils) CAN Socket Error\r\n");
			}

			if(argc == 2)
				strcpy(ifr.ifr_name, argv[1]);
			else strcpy(ifr.ifr_name, "vcan0" );

			ioctl(fdSocketCAN, SIOCGIFINDEX, &ifr);
			
			memset(&addr, 0, sizeof(addr));
			addr.can_family = AF_CAN;
			addr.can_ifindex = ifr.ifr_ifindex;

			if (bind(fdSocketCAN, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				perror("Bind");
				printf("(Fils) CAN Bind Error\r\n");
			}

			/// A filter matches, when <received_can_id> & mask == can_id & mask
			struct can_filter rfilter[3]; // filtres pour 3 ID

			rfilter[0].can_id   = 0x001; 
			rfilter[0].can_mask = 0xFFF;
			rfilter[1].can_id   = 0x017;
			rfilter[1].can_mask = 0xFFF;
			rfilter[2].can_id   = 0x031;
			rfilter[2].can_mask = 0xFFF;

			setsockopt(fdSocketCAN, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

			nbytes = read(fdSocketCAN, &frame, sizeof(struct can_frame));

			if (nbytes < 0) {
				perror("Read");
				printf("(Fils) CAN Close Error\r\n");
			}

			printf("(Fils) 0x%03X [%d] ",frame.can_id, frame.can_dlc);

			for (i = 0; i < frame.can_dlc; i++)
				printf("%02X ",frame.data[i]);

			printf("\r\n");

			
		}

		
	}
	else
	{ // Code exécuté par le processus parent

		while(1)
		{
			static int fdSocketCAN; 
			static struct sockaddr_can addr;
			static struct ifreq ifr;
			static struct can_frame frame;
			static int iDataCanSent = 0;

			if(iDataCanSent > 255)
			{
				iDataCanSent = 0;
			}
			else
			{
				iDataCanSent++;
			}


			/*
			La première étape est de créer un socket. 
			Cette fonction accepte trois paramètres : 
				domaine/famille de protocoles (PF_CAN), 
				type de socket (raw ou datagram) et 
				protocole de socket. 
			la fonction retourne un descripteur de fichier.
			*/
			if ((fdSocketCAN = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) { // Création du socket CAN, de type RAW
				perror("Socket");
				printf("(Pere) CAN Socket Error\r\n");
			}

			/*
			Ensuite, récupérer l'index de l'interface pour le nom de l'interface (can0, can1, vcan0, etc.) 
			que nous souhaitons utiliser. Envoyer un appel de contrôle d'entrée/sortie et 
			passer une structure ifreq contenant le nom de l'interface 
			*/
			if(argc == 2) // si un argument est passé au programme, on l'assigne au nom da l'interface CAN à utiliser
				strcpy(ifr.ifr_name, argv[1]);
			else strcpy(ifr.ifr_name, "vcan0" ); // par défaut l'interface can0

			ioctl(fdSocketCAN, SIOCGIFINDEX, &ifr);
			/*	Alternativement, zéro comme index d'interface, permet de récupérer les paquets de toutes les interfaces CAN.
			Avec l'index de l'interface, maintenant lier le socket à l'interface CAN
			*/

			/*
			
			*/
			memset(&addr, 0, sizeof(addr));
			addr.can_family = AF_CAN;
			addr.can_ifindex = ifr.ifr_ifindex;

			if (bind(fdSocketCAN, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				perror("Bind");
				printf("(Pere) CAN Bind Error\r\n");
			}

			/*
			Envoyer une trame CAN, initialiser une structure can_frame et la remplir avec des données. 
			La structure can_frame de base est définie dans include/linux/can.h  
			*/
			frame.can_id = 0x002;  	// identifiant CAN, exemple: 247 = 0x0F7
			frame.can_dlc = 7;		// nombre d'octets de données
			//sprintf(frame.data, "ABCDEFG");  // données 
			sprintf(frame.data, (char)iDataCanSent);  // données 

			if (write(fdSocketCAN, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
				perror("Write");
				printf("(Pere) CAN Write Error\r\n");
			}

			usleep(10000); //sleep 10000u sec (10ms)

		}
	}


}
