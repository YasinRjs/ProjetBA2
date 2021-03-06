#include "usermanager.h"

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
    myMatchmaking(matchmaking), userChatManager(new ChatManager(userSocket, server)),user(nullptr),gameSock(userSocket->getGameFd())
    {}

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
    //S'il veut se connecter
    do {
        receiveFromClient();
        if (!strcmp(msgClient,"SignIn")){
            signIn();
        }
        //S'il veut s'inscrire
        else if (!strcmp(msgClient,"SignUp")) {
            signUp();
            strcpy(msgClient,"SignUp");
        }
        else if (!strcmp(msgClient,"")){
            printBrutalDisconnect();
        }
    } while(strcmp(msgClient,"SignIn") && strcmp(msgClient,"") && strcmp(msgClient,"Quitter"));
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

void UserManager::sendStringToClient(string msg){
    if (send(new_fd,msg.c_str(),MAXDATASIZE-1,0) == -1) {
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
    // Si le compte est déjà connecté, refuser l'accès
    receiveFromClient();
    string username = msgClient;
    int id = getUserID(msgClient);

    sendOKToClient();
    receiveFromClient();
    if (verifPasswordInDatabase(id,msgClient)){
        sendOKToClient();
        receiveFromClient();
        if (!myServer->isUserConnected(username)){
            sendOKToClient();
            loadCollectionToClient(id);
            searchAndLoadDecks(username);
            myServer->addConnected(username, userChatManager->getRecvChatFd());
            userChatManager->addUsername(username);
            user = myServer->getUserPtr(username);
            myServer->printConnected();	//TOREMOVE later
            startChat();          
            mainMenu();
            strcpy(msgClient,"SignIn");
        }
        else{
            sendNOToClient();
            strcpy(msgClient,"returnMenu");
        }
    }
    else{
        sendNOToClient();
        receiveFromClient();
        sendNOToClient();
        strcpy(msgClient,"returnMenu");
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
            initialiseNewUserDecks(username);
            User* userPtr = new User(to_string(myDatabase->getNbrUsers()),username,0,0);
            myServer->addUserInAllUsers(userPtr);
            addUserInRanking(userPtr);
            flag = 0;
        }
        else{
            sendNOToClient();
            flag = 0;
        }
    }
}

void UserManager::searchAndLoadDecks(string userToLoad){
    fstream fichier;
    string username;
    string deckStr;
    fichier.open("../txt/comptesDecks.txt");
    getline(fichier, username);
    string tmp;
    bool found = false;
    //TODO CHECK IF IT"S THE GOOD USER
    while(!fichier.eof() && !found){
        if (!strcmp(username.c_str(),userToLoad.c_str())){
            found = true;
            for (int i = 0;i<5;i++){
                getline(fichier,deckStr);
                sendStringToClient(deckStr);
                receiveFromClient();
            }
        }
        else{
            for (int i = 0;i<5;i++){
                getline(fichier,tmp);
            }
            getline(fichier,username);
        }
    }
    fichier.close();
}

vector<int> UserManager::createRandomDeck(){ //prendre l'indice de début genre il peut choisir de crérer un deck puis il le compléte aléatoirement
    vector<int> randomDeck;
    vector<int> myCollection;
    myCollection = basicCollectionVect();
    while (randomDeck.size()<20){
        int randomCard = rand()%100;//pour l'instant les ids c'est de 1 à 100
        int c = count (randomCard,randomDeck);
        if (c == 0){
            randomDeck.push_back(randomCard);
        }
        else if (c == 1){
            if (count(randomCard,myCollection)){
                //check si il y a dans sa collection 2 fois la même carte , si c'est le cas le faire
                randomDeck.push_back(randomCard);
            }
        }
    }
    return randomDeck;
}

vector<int> UserManager::basicCollectionVect(){
    vector<int> myCollection;
    for(int i=0;i<100;++i){
        myCollection.push_back(i);
    }
    return myCollection;
}

int UserManager::count(int element,vector<int> vector){//OK
    int c = 0;
    for (unsigned int i = 0;i<vector.size();i++){
        if (vector[i] == element){
            c++;
        }
    }
    return c;
}

void UserManager::initialiseNewUserDecks(string userToInitialise){
    fstream fichier;
    fichier.open("../txt/comptesDecks.txt",ios::app);
    fichier<<userToInitialise<<endl;
    for (int i = 0;i<5;i++){
        vector<int> deck = createRandomDeck();
        for (int j = 0;j<19;j++){
            fichier<<deck[j]<<",";
        }
        fichier<<deck[19];
        fichier<<endl;
    }
    fichier.close();
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
    strcpy(msgClient,"mainMenu");
    while (strcmp(msgClient,"5")){
        receiveFromClient();
        if (!strcmp(msgClient,"1")){
            for (int i=0;i<5;++i){
                receiveFromClient();
                int deckSize = stoi(msgClient);
                for (int j=0;j<deckSize;++j){
                    receiveFromClient();    //recois id
                    string cardParse;
                    cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParseCard();
                    sendStringToClient(cardParse);
                }
            }
            receiveFromClient();
            if (strcmp(msgClient,"")){
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
            strcpy(msgClient,"mainMenu");
        }
        else if (!strcmp(msgClient,"2")){
            collectionMenu();
        }
        else if (!strcmp(msgClient,"3")){
            //userChatManager->startChat();
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

void UserManager::loadCollectionToClient(int ID){
    fstream fichier;
    string collection;
    fichier.open("../txt/comptesCollection.txt");
    for(int i=0;i<ID+1;++i){
        getline(fichier,collection);
    }
    fichier.close();
    sendStringToClient(collection);
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
    sendOKToClient();
    receiveFromClient();
    //Voir Collection
    if (!strcmp(msgClient,"1")){
        sendOKToClient();
        for (int i=0;i<100;++i){
            receiveFromClient();
            string cardParse;
            cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParseCard();
            sendStringToClient(cardParse);
        }
    }
    //Voir Decks
    else if (!strcmp(msgClient,"2")){
        sendOKToClient();
        for (int i=0;i<5;++i){
            receiveFromClient();
            int deckSize = stoi(msgClient);
            for (int j=0;j<deckSize;++j){
                receiveFromClient();    //recois id
                string cardParse;
                cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParseCard();
                sendStringToClient(cardParse);
            }
        }
    }
    //Voir une carte
    else if (!strcmp(msgClient,"3")){
        receiveFromClient();    //Recois l'input
        string cardParse;
        cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParseCard();
        sendStringToClient(cardParse);
        receiveFromClient();    //Quitter ou continuer
    }
    //Création Decks
    else if(!strcmp(msgClient,"4")) {
        vector<int> deck;
        receiveFromClient();
        string username = msgClient;
        receiveFromClient();
        int index = stoi(msgClient);
        for(int i = 0; i < DECKSIZE; i++) {
            receiveFromClient();    //Reçois l'indice de chaque carte du deck
            deck.push_back(stoi(msgClient));
        }
        updateDecksFile(username, deck, index);
    }
    //Suppression Decks
    else if (!strcmp(msgClient,"5")) {
        vector<int> emptyDeck;
        string username;
        int index;
        receiveFromClient();
        username = msgClient;
        receiveFromClient();
        index = stoi(msgClient);
        updateDecksFile(username, emptyDeck, index);
    }
    
    else if (!strcmp(msgClient,"6")){
        //-----------------------------TOUS LES DECKS----------------"OK"--------------------------
        sendOKToClient();
        for (int i=0;i<5;++i){
            receiveFromClient();//Size
            int deckSize = stoi(msgClient);
            for (int j=0;j<deckSize;++j){
                receiveFromClient();    //recois id
                string cardParse;
                cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParseCard();
                sendStringToClient(cardParse);
            }
        }
    
    	//cout<<"msgClient "<<msgClient<<endl;
    	receiveFromClient(); //JE SEND 2 FOIS 1 !!!!!!
        //cout<<"message; "<<msgClient<<endl;
        if (!strcmp(msgClient,"1")){
            sendOKToClient();
            receiveFromClient();//Size
            //cout<<"Size: "<<msgClient<<endl;
            int deckTosize = stoi(msgClient);
            sendOKToClient(); //OK 
            string cardParse;
            for(int i = 0;i < deckTosize;i++ ){
                receiveFromClient();
                cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParserCard();
                sendStringToClient(cardParse);
            }
	
	     receiveFromClient();//TODO
             cout<<"aaaa: "<<msgClient<<endl;
            //TODO PORQUE ??
            if (!strcmp(msgClient,"collectionMenu")){
                //cout<<"haha"<<endl;
                strcpy(msgClient,"mainMenu");
            }
	
	    else if (!strcmp(msgClient,"2")){
	    	cout<<"msgClient "<<msgClient<<endl;
	    	sendOKToClient();
	    	receiveFromClient();
	    	sendOKToClient();
	    	if (!strcmp(msgClient,"stop")){
	    		strcpy(msgClient,"mainMenu");	
	    	}

	    	else{
	    		bool stop = false;
	    		while(!stop){//!strcmp(msgClient,"stop")
	    			//receiveFromClient();    //Recois l'input
                    		//sendOKToClient();
                    		
                    		receiveFromClient();
                    		//cout<<"CARD: "<<msgClient<<endl;
                    		string cardParse;
                    		cardParse = myCardDatabase->getCard(atoi(msgClient)).guiParseCard();
                    		sendStringToClient(cardParse);
                    		if (!strcmp(msgClient,"stop")){//  TODO ||
	    				stop = true;
	    			}	
	    		}
	    
	    	}
	    	if (!strcmp(msgClient,"stop")){
	    		strcpy(msgClient,"mainMenu");	
	    	}
	    }
	    
	    
	    //CRÉER LA FENÊTRE		
	
	}	
	
    	
    	
    	
    	
    	
    	else if (!strcmp(msgClient,"collectionMenu")){
                //cout<<"hihihi"<<endl;
                strcpy(msgClient,"mainMenu");
        }           

    } 		
    
    
    
    //mainMenu
    else if (!strcmp(msgClient,"7")){
        strcpy(msgClient,"mainMenu");
    }
    //Boutton close
    else if (!strcmp(msgClient,"")){
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
    myMatchmaking->addUserInMatchmaking(userPtr, new_fd, deck, userChatManager, gameSock);
}

void* launchChat(void* arg){
    ChatManager* chatPtr = reinterpret_cast<ChatManager*>(arg);
    chatPtr->loop();
    pthread_exit(nullptr);
}

void UserManager::startChat(){
    pthread_create(&currentThread,nullptr,launchChat,reinterpret_cast<void*>(userChatManager));
}

void UserManager::updateDecksFile(string usrname, vector<int> deck, int index) {
    rename("../../GUI/txt/comptesDecks.txt","../../GUI/txt/old.txt");
    ifstream in("../../GUI/txt/old.txt");
    ofstream out("../../GUI/txt/comptesDecks.txt");
    string tmp;
    while(!in.eof()) {
        getline(in,tmp);
        if (!strcmp(tmp.c_str(),usrname.c_str())) {
            out<<tmp<<endl;
            for(int i = 0;i<NUMBEROFDECK;i++) {
                getline(in,tmp);
                if(i==index) {
                    if (deck.size()>0) {
                        for (int j = 0;j<DECKSIZE-1;j++) {
                            out<<deck[j]<<",";
                        }
                    out<<deck[DECKSIZE-1];
                    out<<endl;
                    }
                    else {
                        out<<endl;
                    }
                }
                else {
                    out<<tmp<<endl;
                }
            }
        }
        else{
            if (!in.eof()){
                out<<tmp<<endl;
            }
        }
    }
    in.close();
    out.close();
}
