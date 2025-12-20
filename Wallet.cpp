#include<iostream>
#include"Wallet.h"
#include"CSVReader.h"

Wallet::Wallet()
{
}
     
void Wallet::insertCurrency(std::string type, double amount)
{    
    double balance;
    if (amount < 0)
    {
        throw std::exception{};
    }
     if (currencies.count(type) == 0)
     {   
        balance = 0;
     }  
     else { // is there 
        balance = currencies[type];
     }
     balance += amount; 
     currencies[type] = balance;
}

bool Wallet::containsCurrency(std::string type, double amount)
{
         if (currencies.count(type) == 0)
         return false;
         else{
            return currencies[type] >= amount;
         }
    
    return false;
}

bool Wallet::removeCurrency(std::string type, double amount)
{  
    double balance;
    if (amount < 0)
    {
        return false;
    }
     if (currencies.count(type) == 0)
     {   
        std::cout<< "No currency for"<< type <<std::endl;
        return false;
     }  
     else { // is there - do we have enough 
        if(containsCurrency(type, amount))
        {
            std::cout<<"Removing "<<type <<" : "<<amount<<std::endl;
            currencies[type] -= amount;
            return true;
        }
        else // they have it but not enough
        return false;
     }
}

/*
bool Wallet::removeCurrency(const std::string& type, double amount)
{
    // Requirement 3: throw if amount is negative
    if (amount < 0.0) {
        throw std::invalid_argument("removeCurrency: amount < 0");
    }

    // Locate the currency without creating a new key
    auto it = currencies.find(type);

    // If the currency type doesn't exist, that's "not enough" -> leave unchanged, return true
    if (it == currencies.end()) {
        std::cout << "No currency for " << type << " in wallet (no change)\n";
        return true; // per requirement
    }

    double& balance = it->second;

    // If we don't have enough, leave unchanged, return true
    if (balance < amount) {
        std::cout << "Not enough " << type << " (have " << balance
                  << ", need " << amount << ") — no change\n";
        return true; // per requirement
    }

    // We have enough: subtract and return true
    std::cout << "Removing " << amount << " " << type << '\n';
    balance -= amount;

    // Optional: clean up zero balances
    if (balance == 0.0) {
        currencies.erase(it);
    }

    return true;
}
    */

std::string Wallet::toString()
{   
    std::string s;
    cout<<"Am in Wallet"<<endl;
    for(std::pair<std::string, double> pair : currencies )
    {
        std::string currency = pair.first;
        double amount = pair.second;
        s += currency + " : " + std::to_string(amount) + "\n";

    }
    return s;
}

bool Wallet::canFulfillOrder(OrderBookEntry order)
{   
    std::vector<std::string> currs = CSVReader::tokenise(order.product, '/');
    // ask 
    if(order.orderType == OrderBookType::ask)
    {
        double amount = order.amount;
        std::string currency = currs[0];
        std::cout<<"Wallet::canFulfillOrder "<<currency<<" : "<<amount<<endl;
        return containsCurrency(currency, amount );
        }
   //bid
   if(order.orderType == OrderBookType::bid)
   {
    double amount = order.amount * order.price;
    std::string currency = currs[1];
    std::cout<<"Wallet::canFulfillOrder "<<currency<<" : "<<amount<<endl;
    return containsCurrency(currency, amount);

   }
    return false;
}

void Wallet::processSale(OrderBookEntry& sale)
{ 
     
    std::vector<std::string> currs = CSVReader::tokenise(sale.product, '/');
    // ask 
    if(sale.orderType == OrderBookType::asksale)
    {
        double outgoingAmount = sale.amount;
        std::string outgoingCurrency = currs[0];
        double incomingAmount = sale.amount * sale.price;
        std::string incomingCurrency = currs[1];

        currencies[incomingCurrency] += incomingAmount;
        currencies[outgoingCurrency] -= outgoingAmount;
        }
   //bid
   if(sale.orderType == OrderBookType::bidsale)
   {
        double incomingAmount = sale.amount;
        std::string incomingCurrency = currs[0];
        double outgoingAmount = sale.amount * sale.price;
        std::string outgoingCurrency = currs[1];

        currencies[incomingCurrency] += incomingAmount;
        currencies[outgoingCurrency] -= outgoingAmount;

   }

}

