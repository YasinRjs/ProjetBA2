#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXDATASIZE 200
#define PORT 5555

using namespace std;


class Client{
	char inputPlayer[MAXDATASIZE];
	char msgServer[MAXDATASIZE];
	int sockfd,numbytes;
	struct sockaddr_in my_addr;
	struct hostent *he;
public:
	Client(){};
	Client(int argSize,char* ip);
	~Client(){};

	void verifArgs(int argSize);
	void init(char* ip);
	void welcomeMsg();
	void traceLigne();
	void traceDiese();
	void leaveMsg();
	void askConnection();
	void sendInputToServer();
	void receiveFromServer();
	void signIn();
	void signUp();
	void mainMenu();
};

Client::Client(int argSize,char* ip) : inputPlayer("0"),msgServer("0") {
	verifArgs(argSize);
	init(ip);
	welcomeMsg();
	askConnection();
	sendInputToServer();
	traceDiese();
	if (!strcmp(inputPlayer,"3")){
		leaveMsg();
	}
	else if (strcmp(inputPlayer,"1")){
		//Connexion
		signIn();
	}
	else{
		//Inscription
		signUp();
	}
}

void Client::verifArgs(int argSize){
	if (argSize < 2){
		cout << "Manque adresse ip" << endl;
		exit(1);
	}
}

void Client::init(char* ip){
	if ((he = gethostbyname(ip)) == NULL){
		perror("gethostbyname");
		exit(1);
	}
	if ((sockfd=socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(1);
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(my_addr.sin_zero),'\0',8);
	if (connect(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
	  perror("connect");
	  exit(1);
	}
}

void Client::welcomeMsg(){
	cout << "Bienvenue sur Wizard-Poker !\n- 1 : S'inscrire\n- 2 : Se connecter\n- 3 : Quitter" << endl;
}

void Client::traceLigne(){
	cout << "---------------------------------------" << endl;
}

void Client::traceDiese(){
	cout << "########################################" << endl;
}

void Client::leaveMsg(){
	cout << "Ce jeu est trop difficile pour vous !" << endl;
}

void Client::askConnection(){
	//Réponse du joueur à "se connecter"
	while (strcmp(inputPlayer,"1") && strcmp(inputPlayer,"2") && strcmp(inputPlayer,"3")){
		cout << "Votre réponse (1,2,3) : ";
		cin >> inputPlayer;
	}
}

void Client::sendInputToServer(){
	if (send(sockfd,inputPlayer,MAXDATASIZE-1,0) == -1) {
		perror("recv");
		exit(1);
	}
}

void Client::receiveFromServer(){
	if ((numbytes=recv(sockfd,msgServer,MAXDATASIZE-1,0)) == -1){
		perror("recv");
		exit(1);
	}
	msgServer[numbytes] = '\0';
}

void Client::signIn(){
	strcpy(inputPlayer,"");
	//Demande username
	while (!(strcmp(inputPlayer,""))){
		cout << "Nom d'utilisateur : ";
		cin >> inputPlayer;
		sendInputToServer();
		receiveFromServer();
		//Mot de Passe
		if (!strcmp(msgServer,"OK")){
			cout <<"Mot de passe : ";
			cin >> inputPlayer;
			sendInputToServer();
			receiveFromServer();
			if (!strcmp(msgServer,"OK")){
				cout << "Connection réussie, Have fun !" << endl;
				traceDiese();
				mainMenu();
			}
			else{
				cout << "Mot de passe refusé" << endl;
				traceLigne();
				strcpy(inputPlayer,"");
			}
		}
		else{
			cout << "Nom d'utilisateur refusé" << endl;
			traceLigne();
			strcpy(inputPlayer,"");
		}
	}
}

void Client::signUp(){
	strcpy(inputPlayer,"");
	cout << "~~ Vous allez tenter de vous inscrire ~~" << endl;
	while (!(strcmp(inputPlayer,""))){
		cout << "Nom d'utilisateur : ";
		cin >> inputPlayer;
		sendInputToServer();
		receiveFromServer();
		if (!strcmp(msgServer,"OK")){
			cout << "Mot de passe : ";
			cin >> inputPlayer;
			sendInputToServer();
			cout << "~~~~ Compte créé avec succès ~~~~" << endl;
			cout << "~~~~ Veuillez vous connecter ~~~~" << endl;
			signIn();
		}
		else{
			cout << "Nom d'utilisateur déjà utilisé !" << endl;
			traceLigne();
			strcpy(inputPlayer,"");
		}
	}
}

void Client::mainMenu(){
	cout << "~~~~ Main Menu ~~~~" << endl;
	cout << "- 1 : Recherche d'un adversaire" << endl;
	cout << "- 2 : Collection" << endl;
	cout << "- 3 : Discuter" << endl;
	cout << "- 4 : Classement" << endl;
	cout << "- 5 : Quitter le jeu" << endl;
	cout << "Votre Réponse : ";
	cin >> msgClient;
	sendInputToServer();
}

int main(int argc, char* argv[]){
	Client x = Client(argc,argv[1]);
	return 0;
}



/*
void Client::askConnection(){
	//Réponse du joueur à "se connecter"
	while (strcmp(inputPlayer,"1") && strcmp(inputPlayer,"2")){
		cout << "Votre réponse (1 ou 2) : ";
		cin >> inputPlayer;
	if (strcmp(inputPlayer,"1")){
		cout << "Se connecter" << endl;
	}
	else{
		cout << "S'inscrire"<< endl;
	}
*/
