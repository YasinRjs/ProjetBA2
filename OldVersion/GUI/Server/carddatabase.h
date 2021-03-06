#ifndef CARDDATABASE_H
#define CARDDATABASE_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <cstdlib>
#include <string>

#include "card.h"
using namespace std;

class CardDatabase{
private:
    Card _cards[400];
    string _cardNames[400];
    int _numberCards;
    void getCardsFromFile();

public:
    void addOneCard();
    int getNbrCards();
    int hasCardNamed(string);
    int getID(string);
    int* initialCards();
    string getName(int);
    string getDescription(int);
    int getAttack(int);
    int getHp(int);
    int getCost(int);
    void showAllCards();
    Card getCard(int);
//----------------------------
    CardDatabase();
};

#endif // CARDDATABASE_H
