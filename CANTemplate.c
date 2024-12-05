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
			else strcpy(ifr.ifr_name, "can0" );

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

			rfilter[0].can_id   = 0x001;//0x001
			rfilter[0].can_mask = 0x000;
			rfilter[1].can_id   = 0x017;//0x017
			rfilter[1].can_mask = 0xFFF;
			rfilter[2].can_id   = 0x031;
			rfilter[2].can_mask = 0xFFF;

			setsockopt(fdSocketCAN, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

			nbytes = read(fdSocketCAN, &frame, sizeof(struct can_frame));  //bloque ici
			if (nbytes < 0) {
				perror("Read");
				printf("(Fils) CAN Close Error\r\n");
			}
			
			printf("(Fils) 0x%03X [%d] ",frame.can_id, frame.can_dlc);

			for (i = 0; i < frame.can_dlc; i++)
				printf("%02X ",frame.data[i]);

			printf("\r\n");

			//****************************************************************************************************************/
			// Écrire ici le code pour envoyer frame.data[] sur le port série (NE PEUT PAS ETRE BLOQUANT!!!! il bloque deja quand il lit le can)
			//****************************************************************************************************************/

			
		}

		
	}
	else
	{ // Code exécuté par le processus parent
	

		static int fdSocketCAN; 
		static struct sockaddr_can addr;
		static struct ifreq ifr;
		static struct can_frame frame;
		static unsigned char cDataCanSent[8] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
		static unsigned int prmFois = 0;

		//****************************************************************************************************************/
		// Écrire le code pour lire le port série et mettre les valeurs dans cDataCanSent[] (NE PEUT PAS ETRE BLOQUANT (car il doit envoyer tout les 10ms et il sleep a la fin de la fonction))
		//****************************************************************************************************************/

		

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
			else strcpy(ifr.ifr_name, "can0" ); // par défaut l'interface can0

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
		frame.can_id = 0x034;  	//0x002
		frame.can_dlc = 8;		// nombre d'octets de données
		//sprintf(frame.data, "ABCDEFG");  // données 
		while(1)
		{
			for(int i = 0; i < 8; i++)
			{
				 frame.data[i] = cDataCanSent[i];

			}
			

			if (write(fdSocketCAN, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
				perror("Write");
				printf("(Pere) CAN Write Error\r\n");
			}
			//for (int i = 0; i < frame.can_dlc; i++)
				//printf("%02X ",frame.data[i]);
			
			usleep(100000); //sleep 100000u sec (100ms)

					

		}
	}

}
