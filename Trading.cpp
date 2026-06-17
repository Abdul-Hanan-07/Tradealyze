#include "crow_all.h" 
#include "json.hpp"       
#include "MerkelMain.h"
#include "OrderBook.h"
#include "Wallet.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using json = nlohmann::json;

int main() {
    OrderBook orderBook{"RecordBook_Clean.csv"};
    Wallet wallet;
    std::string currentTime = orderBook.getEarliestTime(); 
    
    // 1. Initialize core simulator funds in the C++ backend
    wallet.insertCurrency("BTC", 10.0);
    wallet.insertCurrency("USDT", 10000.0);

    // --- UI BALANCE TRACKERS (Prevents the NaN Error) ---
    double ui_btc = 10.0;
    double ui_eth = 0.0;
    double ui_usdt = 10000.0;
    double ui_avax = 0.0;

    crow::SimpleApp app;

    CROW_WEBSOCKET_ROUTE(app, "/")
        .onopen([&](crow::websocket::connection& conn) {
            std::cout << "Frontend connected to Tradealyze Engine!\n";
            
            // Send clean, raw numbers to the UI on load
            json balanceMsg;
            balanceMsg["type"] = "balances";
            balanceMsg["btc"] = ui_btc; 
            balanceMsg["eth"] = ui_eth;
            balanceMsg["usdt"] = ui_usdt;
            conn.send_text(balanceMsg.dump());

            std::vector<std::string> products = orderBook.getKnownProducts();
            json productsMsg;
            productsMsg["type"] = "products";
            productsMsg["products"] = products; 
            conn.send_text(productsMsg.dump());
        })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t code) {
            std::cout << "Frontend disconnected from engine.\n";
        })
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            try {
                auto msg = json::parse(data);

                if (msg["action"] == "change_market") {
                    std::string new_product = msg["product"];
                    std::cout << "Market changed by UI to: " << new_product << std::endl;

                    // Fetch the real bids and asks for this specific new product
                    std::vector<OrderBookEntry> liveBids = orderBook.getOrders(OrderBookType::bid, new_product, currentTime);
                    std::vector<OrderBookEntry> liveAsks = orderBook.getOrders(OrderBookType::ask, new_product, currentTime);
                    
                    if (!liveBids.empty() && !liveAsks.empty()) {
                        std::sort(liveBids.begin(), liveBids.end(), OrderBookEntry::comapreByPriceDesc);
                        std::sort(liveAsks.begin(), liveAsks.end(), OrderBookEntry::comapreByPriceAsc);
                        
                        // 1. Send price hints so the order form auto-fills correctly
                        json priceHintMsg;
                        priceHintMsg["type"]      = "price_hint";
                        priceHintMsg["best_bid"]  = liveBids[0].price;
                        priceHintMsg["best_ask"]  = liveAsks[0].price;
                        priceHintMsg["product"]   = new_product;
                        conn.send_text(priceHintMsg.dump());

                        // 2. Send the REAL order book to replace the frontend loading screen
                        json obMsg;
                        obMsg["type"] = "orderbook";
                        obMsg["asks"] = json::array();
                        obMsg["bids"] = json::array();
                        
                        double cumA = 0;
                        for(int i = 0; i < std::min((int)liveAsks.size(), 8); i++) {
                            cumA += liveAsks[i].amount;
                            // Format to strings to match UI expectations cleanly
                            obMsg["asks"].push_back({
                                {"price", std::to_string(liveAsks[i].price)}, 
                                {"amount", std::to_string(liveAsks[i].amount)}, 
                                {"total", std::to_string(cumA)}
                            });
                        }
                        
                        double cumB = 0;
                        for(int i = 0; i < std::min((int)liveBids.size(), 8); i++) {
                            cumB += liveBids[i].amount;
                            obMsg["bids"].push_back({
                                {"price", std::to_string(liveBids[i].price)}, 
                                {"amount", std::to_string(liveBids[i].amount)}, 
                                {"total", std::to_string(cumB)}
                            });
                        }
                        conn.send_text(obMsg.dump());
                    } else {
                        std::cout << "Warning: No CSV data found for " << new_product << " at this time." << std::endl;
                    }
                }

                if (msg["action"] == "submit_order") {
                    std::string side = msg["side"];
                    std::string product = msg["product"];
                    double price = msg["price"];
                    double amount = msg["amount"];
                    
                    OrderBookType type = (side == "buy") ? OrderBookType::bid : OrderBookType::ask;
                    
                    std::vector<std::string> tokens = CSVReader::tokenise(product, '/');
                    std::string baseCurrency = tokens[0]; 
                    std::string quoteCurrency = tokens[1]; 

                    bool canAfford = false;
                    if (type == OrderBookType::bid) {
                        canAfford = wallet.containsCurrency(quoteCurrency, amount * price);
                    } else {
                        canAfford = wallet.containsCurrency(baseCurrency, amount);
                    }

                    if (canAfford) {
                        OrderBookEntry entry{ price, amount, currentTime, product, type, "simuser" };
                        orderBook.insertOrder(entry);
                        std::cout << "SUCCESS: Inserted " << side << " order for " << amount << " " << product << " @ " << price << std::endl;

                        json ackMsg;
                        ackMsg["type"]          = "order_ack";
                        ackMsg["status"]        = "Pending";
                        ackMsg["side"]          = side;
                        ackMsg["product"]       = product;
                        ackMsg["price"]         = price;
                        ackMsg["amount"]        = amount;
                        conn.send_text(ackMsg.dump());

                        std::vector<OrderBookEntry> liveBids = orderBook.getOrders(OrderBookType::bid, product, currentTime);
                        std::vector<OrderBookEntry> liveAsks = orderBook.getOrders(OrderBookType::ask, product, currentTime);
                        if (!liveBids.empty() && !liveAsks.empty()) {
                            std::sort(liveBids.begin(), liveBids.end(), OrderBookEntry::comapreByPriceDesc);
                            std::sort(liveAsks.begin(), liveAsks.end(), OrderBookEntry::comapreByPriceAsc);
                            json priceHintMsg;
                            priceHintMsg["type"]      = "price_hint";
                            priceHintMsg["best_bid"]  = liveBids[0].price;
                            priceHintMsg["best_ask"]  = liveAsks[0].price;
                            priceHintMsg["product"]   = product;
                            conn.send_text(priceHintMsg.dump());
                        }

                    } else {
                        std::cout << "REJECTED: Insufficient wallet balance for order placement." << std::endl;

                        json rejectMsg;
                        rejectMsg["type"]    = "order_ack";
                        rejectMsg["status"]  = "Rejected ✗";
                        rejectMsg["side"]    = side;
                        rejectMsg["product"] = product;
                        rejectMsg["price"]   = price;
                        rejectMsg["amount"]  = amount;
                        conn.send_text(rejectMsg.dump());
                    }
                }
                
                if (msg["action"] == "next_timeframe") {
                    std::vector<std::string> products = orderBook.getKnownProducts();
                    bool dynamicWalletUpdated = false;

                    for (std::string& p : products) {
                        std::vector<OrderBookEntry> sales = orderBook.matchAsksToBids(p, currentTime);
                        
                        std::vector<std::string> tokens = CSVReader::tokenise(p, '/');
                        std::string baseAsset = tokens[0];
                        std::string quoteAsset = tokens[1];

                        for (OrderBookEntry& sale : sales) {
                            if (sale.username == "simuser") {
                                dynamicWalletUpdated = true;
                                
                                if (sale.orderType == OrderBookType::bidsale) {
                                    wallet.insertCurrency(baseAsset, sale.amount);
                                    wallet.removeCurrency(quoteAsset, sale.amount * sale.price);
                                    
                                    if (baseAsset == "ETH") ui_eth += sale.amount;
                                    if (baseAsset == "BTC") ui_btc += sale.amount;
                                    if (baseAsset == "AVAX") ui_avax += sale.amount;
                                    if (quoteAsset == "BTC") ui_btc -= (sale.amount * sale.price);
                                    if (quoteAsset == "USDT") ui_usdt -= (sale.amount * sale.price);
                                    
                                } else if (sale.orderType == OrderBookType::asksale) {
                                    wallet.removeCurrency(baseAsset, sale.amount);
                                    wallet.insertCurrency(quoteAsset, sale.amount * sale.price);
                                    
                                    if (baseAsset == "ETH") ui_eth -= sale.amount;
                                    if (baseAsset == "BTC") ui_btc -= sale.amount;
                                    if (baseAsset == "AVAX") ui_avax -= sale.amount;
                                    if (quoteAsset == "BTC") ui_btc += (sale.amount * sale.price);
                                    if (quoteAsset == "USDT") ui_usdt += (sale.amount * sale.price);
                                }
                            }

                            json tradeMsg;
                            tradeMsg["type"] = "trade";
                            tradeMsg["tx"]["time"] = currentTime.substr(11, 8); 
                            tradeMsg["tx"]["side"] = (sale.orderType == OrderBookType::bidsale) ? "BUY" : "SELL"; 
                            tradeMsg["tx"]["product"] = p;
                            tradeMsg["tx"]["price"] = sale.price;
                            tradeMsg["tx"]["amount"] = sale.amount;
                            tradeMsg["tx"]["total"] = sale.price * sale.amount;
                            tradeMsg["tx"]["status"] = "Filled"; 
                            conn.send_text(tradeMsg.dump());
                        }
                    }
                    
                    if (dynamicWalletUpdated) {
                        json updateBalMsg;
                        updateBalMsg["type"] = "balances";
                        updateBalMsg["btc"] = ui_btc; 
                        updateBalMsg["eth"] = ui_eth;
                        updateBalMsg["usdt"] = ui_usdt;
                        conn.send_text(updateBalMsg.dump());
                    }

                    currentTime = orderBook.getNextTime(currentTime);
                }

            } catch (const std::exception& e) {
                std::cerr << "Data structure error: " << e.what() << std::endl;
            }
        });

    std::cout << "Tradealyze backend running live on port 9002...\n";
    app.port(9002).multithreaded().run();
}