#include<iostream>
#include<string>
#include<vector>
#include<limits>
#include"OrderBookEntry.h"
#include"MerkelMain.h"
#include"CSVReader.h"
#include"Wallet.h"

int main()
{   
   MarkelMain app{};
   app.init();
   CSVReader::readCSV("RecordBook.csv");

    //  Wallet wallet;
    //  wallet.insertCurrency("BTC", 10);
    //  wallet.insertCurrency("USDT", 10000);
    //  std::cout<< wallet.toString()<<std::endl;

    //  wallet.removeCurrency("USDT", 1000);

    //  std::cout<< wallet.toString()<<std::endl;

   }