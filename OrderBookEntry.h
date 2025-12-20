#pragma once

#include<string>
#include<iostream>

enum class OrderBookType{bid , ask, unknown, asksale, bidsale};

   // Start of OrderBookEntry class

class OrderBookEntry
{
    public:
    double price;
    double amount;  
    std::string product;
    std::string timestamp{};
    OrderBookType orderType;

     // Fetch Data through constructor parameters

    OrderBookEntry(double _price,
                   double _amount, 
                   std::string _timestamp, 
                   std::string _product,
                   OrderBookType _orderType,
                   std::string username = "dataset");

   static OrderBookType stringToOrderBookType(std::string s);

   static bool comapreByTimestamp(OrderBookEntry& e1, OrderBookEntry& e2)
   {
        return e1.timestamp < e2.timestamp;
   }
    static bool comapreByPriceAsc(OrderBookEntry& e1, OrderBookEntry& e2)
   {
        return e1.price < e2.price;
   }
   static bool comapreByPriceDesc(OrderBookEntry& e1, OrderBookEntry& e2)
   {
        return e1.price > e2.price;
   }
   
   std::string username;

};
