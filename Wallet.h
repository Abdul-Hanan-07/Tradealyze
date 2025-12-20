#pragma once
#include<string>
#include<map>
#include<iostream>
#include"OrderBookEntry.h"


class Wallet
{
    public: 
    Wallet();
    // Insert currency in wallet
    void insertCurrency(std::string type, double amount);
    // Remove currency from the wallet

    bool removeCurrency(std::string type, double amount);

    // Check if the wallet contains this muchy currency or more 
    bool containsCurrency(std::string type, double amount);
    // checks if the wallet can cope with this ask and bid.
    bool canFulfillOrder(OrderBookEntry order);
    // update the contents of the wallet 
    // assumes the order was made by the owner of the wallet
    void processSale(OrderBookEntry& sale);


    // generate a string representation of the wallet.
    std::string toString();  


    private:
     std::map<std::string, double> currencies;

};