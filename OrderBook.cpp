#include <iostream>
#include "CSVReader.h"
#include <map>
#include <vector>
#include <limits>
#include <algorithm>
#include "OrderBook.h"

using namespace std;

// construct, reading a csv file
OrderBook::OrderBook(string filename)
{
  orders = CSVReader::readCSV(filename);
}

// return vector of all known products in their data set
vector<string> OrderBook::getKnownProducts()
{
  vector<string> products;
  map<string, bool> prodMap;

  for (OrderBookEntry &e : orders)
  {
    prodMap[e.product] = true;
  }

  // now flatten the map to a vector of strings
  for (auto const &e : prodMap)
  {
    products.push_back(e.first);
  }

  return products;
}

// return vector of Orders according to the sent filters
vector<OrderBookEntry> OrderBook::getOrders(OrderBookType type,
                                            string product,
                                            string timestamp)
{
  vector<OrderBookEntry> orders_sub;
  for (OrderBookEntry &e : orders)
  {
    if (e.orderType == type &&
        e.product == product &&
        e.timestamp == timestamp)
    {
      orders_sub.push_back(e);
    }
  }
  return orders_sub;
}

double OrderBook::getHighPrice(vector<OrderBookEntry> &orders)
{
  if (orders.empty()) return 0.0;
  double max = orders[0].price;
  for (OrderBookEntry &e : orders)
  {
    if (e.price > max)
      max = e.price;
  }
  return max;
}

double OrderBook::getLowerPrice(vector<OrderBookEntry> &orders)
{
  if (orders.empty()) return 0.0;
  double min = orders[0].price;
  for (OrderBookEntry &e : orders)
  {
    if (e.price < min)
      min = e.price;
  }
  return min;
}

string OrderBook::getEarliestTime()
{
  return orders[0].timestamp;
}

 string OrderBook::getNextTime(string timestamp)
 {
   string next_timestamp = "";
   for (OrderBookEntry& e : orders)
   {
     if (e.timestamp > timestamp)
     {
       next_timestamp = e.timestamp;
       break;
     }
   }
   if (next_timestamp == "")
   {
     next_timestamp = orders[0].timestamp;
   }
   return next_timestamp;
 }

void OrderBook::insertOrder(OrderBookEntry& order)
{
  orders.push_back(order);
  sort(orders.begin(), orders.end(), OrderBookEntry::comapreByTimestamp);
}

vector<OrderBookEntry> OrderBook::matchAsksToBids(string product, string timestamp)
{
  vector<OrderBookEntry> asks = getOrders(OrderBookType::ask, product, timestamp);
  vector<OrderBookEntry> bids = getOrders(OrderBookType::bid, product, timestamp);
  vector<OrderBookEntry> sales;

  // 👇 SAFETY SHIELD: Stop the crash if there is no data for this exact second 👇
  if (asks.empty() || bids.empty()) {
      // std::cout << "Skipping matching: No asks or bids available at this exact second." << std::endl;
      return sales; 
  }
  // 👆 END OF SAFETY SHIELD 👆

  // sort asks lowest first
  std::sort(asks.begin(), asks.end(), OrderBookEntry::comapreByPriceAsc);
  // sort bids highest first
  std::sort(bids.begin(), bids.end(), OrderBookEntry::comapreByPriceDesc);
  
  // std::cout << "max ask " << asks[asks.size()-1].price << std::endl;
  // std::cout << "min ask " << asks[0].price << std::endl;
  // std::cout << "max bid " << bids[0].price << std::endl;
  // std::cout << "min bid " << bids[bids.size()-1].price << std::endl;

  for(OrderBookEntry& ask : asks)
  { 
    for(OrderBookEntry& bid: bids)
    {
      if(bid.price >= ask.price)
      {
    //    std::cout << "bid price is right " << std::endl;
        
        OrderBookEntry sale{ask.price, 0, timestamp, product, OrderBookType::asksale};
       
        if(bid.username == "simuser")
        {
          sale.username = "simuser";
          sale.orderType =  OrderBookType::bidsale;
        }
        if (ask.username == "simuser") // Fixed typo here (was siuser)
        {
           sale.username = "simuser";
           sale.orderType = OrderBookType::asksale;
        }

        if(bid.amount == ask.amount )
        {
          sale.amount = ask.amount;
          sales.push_back(sale);
          bid.amount = 0;
          break;
        }

        if(bid.amount > ask.amount)
        {
          sale.amount = ask.amount;
          sales.push_back(sale);
          bid.amount = bid.amount - ask.amount;
          break;
        }

        if(bid.amount < ask.amount && bid.amount > 0)
        {
           sale.amount = bid.amount;
           sales.push_back(sale);
           ask.amount = ask.amount - bid.amount;
           bid.amount=0;
           continue;
        }
      }
    }
  }
  return sales;
}