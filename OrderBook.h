#pragma once

#include"OrderBookEntry.h"
#include"CSVReader.h"
#include<string>
#include<vector>

using namespace std;


class OrderBook
{
    public: 
// construct, reading a csv file
    OrderBook(string filename);
    // retrn vector of all known produucts inn their data set
    vector<string> getKnownProducts();
    // return vector of Orders according to the sent filters 
    vector<OrderBookEntry> getOrders(OrderBookType type,
                                    string prduct,
                                    string timestamp);
    // Return the earliest time inn the orderbook
    string getEarliestTime();

    // Return the next time after the sent time in the orderBook
    // if there is no timestamp, wraps around to the start
    string getNextTime(string timestamp);
   
    void insertOrder(OrderBookEntry& order);

     vector<OrderBookEntry> matchAsksToBids(string product, string timestamp);

    static double getHighPrice( vector<OrderBookEntry>& orders);
    static double getLowerPrice( vector<OrderBookEntry>& orders);

    // Computes Volume-Weighted Average Price over the provided orders
  //  static double getVWAP(const std::vector<OrderBookEntry>& orders);
    private:
    vector<OrderBookEntry> orders;

};