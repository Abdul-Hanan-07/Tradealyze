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
            e.timestamp <= timestamp)   // <= keeps simuser orders inserted at currentTime
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

    if (asks.empty() || bids.empty()) {
        return sales; 
    }

    // sort asks lowest first
    std::sort(asks.begin(), asks.end(), OrderBookEntry::comapreByPriceAsc);
    // sort bids highest first
    std::sort(bids.begin(), bids.end(), OrderBookEntry::comapreByPriceDesc);
  
    for(OrderBookEntry& ask : asks)
    { 
        if (ask.amount <= 0) continue; // FIX 2: Safety Check

        for(OrderBookEntry& bid: bids)
        {
            if (bid.amount <= 0) continue; // FIX 2: Safety Check

            if(bid.price >= ask.price)
            {
                OrderBookEntry sale{ask.price, 0, timestamp, product, OrderBookType::asksale};
               
                // FIX: Use else-if so the second check cannot overwrite the first.
                // If bid is simuser → bidsale (user bought). If ask is simuser → asksale (user sold).
                // Both cannot be simuser simultaneously in a real sim, but guard it properly.
                if (bid.username == "simuser") {
                    sale.username  = "simuser";
                    sale.orderType = OrderBookType::bidsale;
                } else if (ask.username == "simuser") {
                    sale.username  = "simuser";
                    sale.orderType = OrderBookType::asksale;
                }

                if(bid.amount == ask.amount)
                {
                    sale.amount = ask.amount;
                    sales.push_back(sale);
                    bid.amount = 0;
                    ask.amount = 0;
                    break; // FIX 2: Ask is fully filled, break out to the next ask
                }
                else if(bid.amount > ask.amount)
                {
                    sale.amount = ask.amount;
                    sales.push_back(sale);
                    bid.amount = bid.amount - ask.amount;
                    ask.amount = 0;
                    break; // FIX 2: Ask is fully filled, break out to the next ask
                }
                else if(bid.amount < ask.amount && bid.amount > 0)
                {
                    sale.amount = bid.amount;
                    sales.push_back(sale);
                    ask.amount = ask.amount - bid.amount;
                    bid.amount = 0;
                    continue; // FIX 2: Bid is empty, keep processing the remaining ask amount
                }
            }
        }
    }

    // FIX 2 (Crucial): Sync the modified amounts back to the main orders vector
    for (OrderBookEntry& originalOrder : orders)
    {
        if (originalOrder.timestamp <= timestamp && originalOrder.product == product)
        {
            for (const auto& ask : asks) {
                if (originalOrder.username == ask.username && originalOrder.price == ask.price && originalOrder.orderType == OrderBookType::ask) {
                    originalOrder.amount = ask.amount;
                }
            }
            for (const auto& bid : bids) {
                if (originalOrder.username == bid.username && originalOrder.price == bid.price && originalOrder.orderType == OrderBookType::bid) {
                    originalOrder.amount = bid.amount;
                }
            }
        }
    }

    // FIX 2 (Crucial): Clean up fully filled orders so they don't loop
    orders.erase(std::remove_if(orders.begin(), orders.end(), [](OrderBookEntry& e) {
        return e.amount <= 0;
    }), orders.end());

    return sales;
}