#include "UserManager.hpp"

void* newConnection(void* arg){
	UserManager* managerPtr = reinterpret_cast<UserManager*>(arg);
	managerPtr->printNewConnection();
	managerPtr->treatUser();
	managerPtr->printEndConnection();
	delete managerPtr;
	pthread_exit(nullptr);
}

UserManager::UserManager() : my_addr(sockaddr_in()),new_fd(0),myDatabase(nullptr),myCardDatabase(nullptr),
myRanking(nullptr),myServer(nullptr),myMatchmaking(nullptr),userChatManager(nullptr),user(nullptr){}

UserManager::UserManager(ClientSocket* userSocket,Database* database,CardDatabase* cardDatabase, Ranking* ranking, Matchmaking* matchmaking, Server* server)
	: my_addr(userSocket->getAddr()),new_fd(userSocket->getFd()),
	myDatabase(database), myCardDatabase(cardDatabase), myRanking(ranking), myServer(server),
	myMatchmaking(matchmaking), userChatManager(new ChatManager(userSocket, server)),user(nullptr){}

UserManager::UserManager(const UserManager&) : my_addr(sockaddr_in()),new_fd(0),myDatabase(nullptr),myCardDatabase(nullptr),
myRanking(nullptr),myServer(nullptr),myMatchmaking(nullptr),userChatManager(nullptr),user(nullptr){}

UserManager& UserManager::operator=(const UserManager&){
	return *this;
}

UserManager::~UserManager(){
	delete userChatManager;
}


void UserManager::start(){
	pthread_create(&currentThread,nullptr,newConnection,reinterpret_cast<void*>(this));
}

void UserManager::treatUser(){
	receiveFromClient();
	//S'il veut se connecter
	if (!strcmp(msgClient,"2")){
		signIn();
	}
	//S'il veut s'inscrire
	else if (!strcmp(msgClient,"1")) {
		signUp();
		signIn();
	}
	else if (!strcmp(msgClient,"")){
		printBrutalDisconnect();
	}
}

void UserManager::receiveFromClient(){
	if ((numbytes=recv(new_fd,msgClient,MAXDATASIZE-1,0)) == -1){
		perror("recv");
	}
	msgClient[numbytes] = '\0';
}

void UserManager::sendOKToClient(){
	if ((send(new_fd,"OK",MAXDATASIZE-1,0)) == -1){
		perror("recv");
		exit(1);
	}
}

void UserManager::sendNOToClient(){
	if ((send(new_fd,"NO",MAXDATASIZE-1,0)) == -1){
		perror("recv");
		exit(1);
	}
}

void UserManager::sendIDToClient(int id){
	string test;
	test = to_string(id);
	char sendID[MAXDATASIZE];
	for(unsigned i=0;i<test.size();++i){
		sendID[i] = test[i];
	}
	if ((send(new_fd,sendID,MAXDATASIZE-1,0)) == -1){
		perror("recv");
		exit(1);
	}	
}

void UserManager::printNewConnection(){
	cout << "Connection de : " << inet_ntoa(my_addr.sin_addr) << endl;
}

void UserManager::printEndConnection(){
	cout << "--> Fin de la connection avec : " << inet_ntoa(my_addr.sin_addr) << endl;
}

void UserManager::printBrutalDisconnect(){
	cout << "~~> Deconnection brutale de : " << inet_ntoa(my_addr.sin_addr) << endl;
}

void UserManager::signIn(){
	int flag = 1;
	int id;
	while (flag){
		receiveFromClient();
// Si le compte est déjà connecté, refuser l'accès
		if (hasUserInDatabase(msgClient)){
			string username = msgClient;
			id = getUserID(msgClient);
			sendOKToClient();
			receiveFromClient();
			if (verifPasswordInDatabase(id,msgClient)){
				sendOKToClient();
				receiveFromClient();
				if (!myServer->isUserConnected(username)){
					sendOKToClient();
					flag = 0;
					myServer->addConnected(username, userChatManager->getRecvChatFd());
					userChatManager->addUsername(username);
					user = myServer->getUserPtr(username); 
					myServer->printConnected();	//TOREMOVE later
					sendIDToClient(id);
					mainMenu();
				}
				else{
					sendNOToClient();
				}
			}
			else{
				sendNOToClient();
			}
		}
		else if (!strcmp(msgClient,"")){
			printBrutalDisconnect();
			flag = 0;
		}
		else{
			sendNOToClient();
		}
	}
}

void UserManager::signUp(){
	//TODO inscription
	int flag = 1;
	while (flag){
		receiveFromClient();
		if (!strcmp(msgClient,"")){
			flag = 0;
		}
		else if (!hasUserInDatabase(msgClient)){
			sendOKToClient();
			string username = msgClient;
			receiveFromClient();
			string pwd = msgClient;
			addUserInDatabase(username,pwd);
			User* userPtr = new User(to_string(myDatabase->getNbrUsers()),username,0,0);
			myServer->addUserInAllUsers(userPtr);
			addUserInRanking(userPtr);
			flag = 0;
		}
		else{
			sendNOToClient();
		}
	}
}

void UserManager::sendRankingToClient(){
	if ((send(new_fd,myRanking->getRanking().c_str(),myRanking->getRanking().size(),0)) == -1){
		perror("recv");
		exit(1);
	}
}

int UserManager::getUserID(string username){
	return myDatabase->getID(username);
}

int UserManager::verifPasswordInDatabase(int id,char* pwd){
	return myDatabase->isCorrectPassword(id,pwd);
}

int UserManager::hasUserInDatabase(string username){
	return myDatabase->hasUserNamed(username);
}

void UserManager::addUserInDatabase(string username,string pwd){
	myDatabase->addUser(username,pwd);
}

void UserManager::addUserInRanking(User* user){
	myRanking->addPlayer(user);
}

void UserManager::mainMenu(){

	strcpy(msgClient,"");
	while (strcmp(msgClient,"5")){
		receiveFromClient();
		if (!strcmp(msgClient,"1")){
			receiveFromClient();
			vector<int> chosenDeck = convertMsgToDeck(msgClient);
			int nbrCardInDeck = 20;
			vector<Card> vectorCards;
			for(int i=0;i<nbrCardInDeck;++i){
				vectorCards.push_back(myCardDatabase->getCard(chosenDeck[i]));
			}
			GameDeck chosenGameDeck = GameDeck(vectorCards);
			findOpponent(chosenGameDeck);
			strcpy(msgClient,"mainMenu");
		}
		else if (!strcmp(msgClient,"2")){
			collectionMenu();
		}
		else if (!strcmp(msgClient,"3")){
			userChatManager->startChat();
		}
		else if (!strcmp(msgClient,"4")){
			sendRankingToClient();
		}
		else if (!strcmp(msgClient,"5")){
			DisconnectUser();
		}
		else if (!strcmp(msgClient,"")){
			strcpy(msgClient,"5");
			printBrutalDisconnect();
			DisconnectUser();

		}
		else{
			strcpy(msgClient,"mainMenu"); // TO CHANGE LATER - Jacky
		}
	}
}

vector<int> UserManager::convertMsgToDeck(string msg){
	vector<int> deck;

	string token;
	char delimiter = ',';
	for(unsigned i=0;i<msg.size();++i){
		if (msg[i] == delimiter){
			deck.push_back(atoi(token.c_str()));
			token = "";
		}
		else{
			token += msg[i];
		}
	}
	deck.push_back(atoi(token.c_str()));

	return deck;
}

void UserManager::collectionMenu(){
	receiveFromClient();
	if (!strcmp(msgClient,"3") || !strcmp(msgClient,"4")){
		strcpy(msgClient,"mainMenu");
	}
	else if (!strcmp(msgClient,"5")){
		strcpy(msgClient,"mainMenu");
	}
	else if (!strcmp(msgClient,"6")){
		strcpy(msgClient,"mainMenu");
	}
}

void UserManager::DisconnectUser(){
	int chat_fd = userChatManager->getRecvChatFd();
	myServer->removeDisconnected(chat_fd);
}

//GameDeck deck
void UserManager::findOpponent(GameDeck deck){
	User* userPtr = user;
	myMatchmaking->addUserInMatchmaking(userPtr, new_fd, deck, userChatManager);
}