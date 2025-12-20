#include "MerkelMain.h"
#include <iostream>
#include <vector>
#include "OrderBookEntry.h"
#include "CSVReader.h"

using namespace std;

MarkelMain::MarkelMain()
{
    
}
void MarkelMain::init()
{
  int input;
 wallet.insertCurrency("BTC", 10);

  currentTime = orderBook.getEarliestTime();
  while(true)
  { 
    printmenu();
    input= selectoption();
    processuseroption(input);
    
  }
}
 
// void MarkelMain::loadOrderbook()
// {
     
//   // 1st Vec data
//     orders.push_back(OrderBookEntry {1000,
//                          0.02,
//                          "BTC/USDT",
//                         "2020/03/17 17:01:24.884492",
//                         OrderBookType::bid }   );
//       // 2nd Vector data
//     orders.push_back(OrderBookEntry {2000,
//                          0.02,
//                          "BTC/USDT",
//                         "2020/03/17 17:01:24.884492",
//                         OrderBookType::bid }   );

// }
void MarkelMain::printmenu()
{  
     std::cout<<endl;
    std::cout<<"1: Print Help" <<std::endl;
    
    std::cout<<"2: Print exchange stats" <<std::endl;
    
    std::cout<<"3: Make an ask" <<std::endl;
    
    std::cout<<"4: Mke a bid" <<std::endl;
    
    std::cout<<"5: Print wallet" <<std::endl;
        
    std::cout<<"6: Continue" <<std::endl;

    std::cout<<"================"<<std::endl;
   
    std::cout<<"Current time is: "<<currentTime<<std::endl;
}

int MarkelMain::selectoption()
{  
    int userOption=0;
    string line;
    std::cout<<"Type in 1-6 : "<<std::endl;
    getline(cin, line);
    try{
    userOption= stoi(line);
    }
    catch(const exception& e)
    {
        //Nothing want to write there
    }

    std::cout<<"You Choose : "<<userOption<<std::endl;
    return userOption;

}

void MarkelMain::invalidchoice()
{
    std::cout<<"Invalid choice. Choose 1-6"<<std::endl;
}

void  MarkelMain::help()
{
    std::cout<<"Help - your aim is to make money. Analyse the market and make bids and offer "<<std::endl;
} 
     
void MarkelMain::market()
{   
    for (string const& p : orderBook.getKnownProducts())
    {     
        cout<<"Product: "<< p <<endl;
        vector<OrderBookEntry> entries = orderBook.getOrders(OrderBookType::ask,
                                                               p, currentTime );
        cout<<"Asks Seen:"<<entries.size()<<endl;
        cout<<"Max Ask: "<<OrderBook::getHighPrice(entries) <<endl;
        cout<<"Min Ask: "<<OrderBook::getLowerPrice(entries) <<endl;

    }
  
}  
    
void MarkelMain::enterAsk()
{
    std::cout<<"Make an ask - enter the amount: product, price, amount, eg ETH/BTC, 200, 0.5"<<std::endl;
    std::string input;
    std::getline(std::cin, input);
   
    std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
    if(tokens.size() != 3)
    {
        std::cout<<"MerkelMain::enterAsk Bad Input"<<input<<endl;

    }
    else{
        try {
        OrderBookEntry obe = CSVReader::stringToOBE(
                                                   tokens[1],
                                                   tokens[2],
                                                   currentTime,
                                                   tokens[0],
                                                   OrderBookType::ask
                                                     );
         obe.username = "simuser";
         if (wallet.canFulfillOrder(obe))
         {
            cout<<"Wallet loooks good. "<<endl;

         orderBook.insertOrder(obe);
        }
        else
        {
            cout<<"Wallet has insufficient funds. "<<endl;
        }

        }
        catch (const exception& e)
        {
            cout<<" MarkelMain::enterAsk Bad Input"<<endl;
        }
    }
 
} 
    
void MarkelMain::enterBid()
{
std::cout<<"Make a bid - enter the amount: product, price, amount, eg ETH/BTC, 200, 0.5"<<std::endl;
    std::string input;
    std::getline(std::cin, input);
   
    std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
    if(tokens.size() != 3)
    {
        std::cout<<"MerkelMain::enterBid Bad Input"<<input<<endl;

    }
    else{
        try {
        OrderBookEntry obe = CSVReader::stringToOBE(
                            tokens[1],
                            tokens[2],  
                            currentTime,
                            tokens[0],
                            OrderBookType::bid
                                );
          obe.username = "simuser";

         if (wallet.canFulfillOrder(obe))
         {
            cout<<"Wallet loooks good. "<<endl;

         orderBook.insertOrder(obe);
        }
        else
        {
            cout<<"Wallet has insufficient funds. "<<endl;
        }

        }
        catch (const exception& e)
        {
            cout<<" MarkelMain::enterBid Bad Input"<<endl;
        }
    }}  
    
void MarkelMain::printwallet()
{
    std::cout<<wallet.toString()<<std::endl;
}
        
void MarkelMain::gotoNextTimeframe()
{ 
    std::cout<<"Going to next time frame"<<std::endl;
    std::vector<OrderBookEntry> sales = orderBook.matchAsksToBids("ETH/BTC", currentTime);
    std::cout<<"Sales: "<<sales.size()<<endl;
    for(OrderBookEntry& sale : sales)
    {
        std::cout<<"Sale price: "<< sale.price << "Amount " << sale.amount <<endl;
        if(sale.username == "simuser")
        {
            //update the wallet
            wallet.processSale(sale);

        }
    }
    currentTime = orderBook.getNextTime(currentTime);

}

void MarkelMain::processuseroption(int selectOption)
{
    
if(selectOption==0)
{
    invalidchoice();
}

if(selectOption==1)
{
    help();
} 
     
if(selectOption==2)
{
    market();
}  
    
if(selectOption==3)
{
    enterAsk();
}  
if(selectOption==4)
{
    enterBid();
}  
    
if(selectOption==5)
{
    printwallet();
}
        
if(selectOption==6)
{
 gotoNextTimeframe();
}

}
