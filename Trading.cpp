#include "crow_all.h" 
#include "json.hpp"       
#include "MerkelMain.h"
#include "OrderBook.h"
#include "Wallet.h"
#include <iostream>
#include <vector>
#include <string>

using json = nlohmann::json;

int main() {
    OrderBook orderBook{"RecordBook.csv"};
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
                    } else {
                        std::cout << "REJECTED: Insufficient wallet balance for order placement." << std::endl;
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
                                    // User Bought: Gain Base, Lose Quote
                                    wallet.insertCurrency(baseAsset, sale.amount);
                                    wallet.removeCurrency(quoteAsset, sale.amount * sale.price);
                                    
                                    // Sync UI Variables safely
                                    if (baseAsset == "ETH") ui_eth += sale.amount;
                                    if (baseAsset == "BTC") ui_btc += sale.amount;
                                    if (baseAsset == "AVAX") ui_avax += sale.amount;
                                    if (quoteAsset == "BTC") ui_btc -= (sale.amount * sale.price);
                                    if (quoteAsset == "USDT") ui_usdt -= (sale.amount * sale.price);
                                    
                                } else if (sale.orderType == OrderBookType::asksale) {
                                    // User Sold: Lose Base, Gain Quote
                                    wallet.removeCurrency(baseAsset, sale.amount);
                                    wallet.insertCurrency(quoteAsset, sale.amount * sale.price);
                                    
                                    // Sync UI Variables safely
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