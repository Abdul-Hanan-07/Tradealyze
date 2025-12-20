#pragma once

#include<vector>
#include"OrderBookEntry.h"
#include"OrderBook.h"
#include"Wallet.h"

class MarkelMain
{
    public:
    MarkelMain();
    // Call this to start the sim
    void init();
    private:
    void printmenu();
    int selectoption();
    void invalidchoice();
    void help();   
    void market();
    void enterAsk();
    void enterBid();
    void printwallet();
    void gotoNextTimeframe();
    void processuseroption(int selectOption);


    string currentTime;

    OrderBook orderBook{"RecordBook.csv"};
    
    Wallet wallet;

};