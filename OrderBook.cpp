#include <iostream>
#include "CSVReader.h"
#include <map>
#include <vector>
#include<limits>
#include<algorithm>
#include "OrderBook.h"
// #include <limits>

using namespace std;

// construct, reading a csv file
OrderBook::OrderBook(string filename)
{
  orders = CSVReader::readCSV(filename);
}
// retrn vector of all known produucts inn their data set
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

// double OrderBook::getVWAP(const std::vector<OrderBookEntry>& orders)
// {
//     if (orders.empty()) 
//     return std::numeric_limits<double>::quiet_NaN();

//     double weightedSum = 0.0;
//     double volumeSum   = 0.0;

//     for (const auto& e : orders)
//     {
//         // Ignore zero/negative amounts defensively
//         if (e.amount > 0.0) {
//             weightedSum += e.price * e.amount;
//             volumeSum   += e.amount;
//         }
//     }

//     if (volumeSum == 0.0) return std::numeric_limits<double>::quiet_NaN();
//     return weightedSum / volumeSum;
// }

void OrderBook::insertOrder(OrderBookEntry& order)
{
  orders.push_back(order);
  sort(orders.begin(), orders.end(), OrderBookEntry::comapreByTimestamp);
}

// asks = orderbook.asks
vector<OrderBookEntry> OrderBook::matchAsksToBids(string product, string timestamp)
{
  vector<OrderBookEntry> asks = getOrders(OrderBookType::ask,
                                          product,
                                          timestamp);
// bids = orderbook.bids
vector<OrderBookEntry> bids = getOrders(OrderBookType::bid,
                                          product,
                                          timestamp);
// sales[]
vector<OrderBookEntry> sales;
// sort asks lowest first
std::sort(asks.begin(), asks.end(), OrderBookEntry::comapreByPriceAsc);
// sort bids highest first
std::sort(bids.begin(), bids.end(), OrderBookEntry::comapreByPriceDesc);
 // for ask in asks:
    std::cout << "max ask " << asks[asks.size()-1].price << std::endl;
    std::cout << "min ask " << asks[0].price << std::endl;
    std::cout << "max bid " << bids[0].price << std::endl;
    std::cout << "min bid " << bids[bids.size()-1].price << std::endl;

// for ask in asks:
for(OrderBookEntry& ask : asks)
{ // for bids in bid:
  for(OrderBookEntry& bid: bids)
  {
    if(bid.price >= ask.price)
    {
      std::cout << "bid price is right " << std::endl;
      
      OrderBookEntry sale{
        ask.price, 0, timestamp, product,
        OrderBookType::asksale};
     
      if(bid.username== "simuser")
      {
       sale.username = "simuser";
       sale.orderType =  OrderBookType::bidsale;
      }
      if (ask.username == "siuser")
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

    if(bid.amount < ask.amount &&
       bid.amount > 0)
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