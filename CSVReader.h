#pragma once
#include"OrderBookEntry.h"
#include<vector>
#include<iostream>
#include<string>
using namespace std;


class CSVReader
{
    public:

    CSVReader();

    static vector<OrderBookEntry> readCSV(string csvFile);
    static vector<string> tokenise(string csvline, char separator); 

    static OrderBookEntry stringToOBE(string price,
                                       string amount,  
                                       string timestamp,
                                       string product,
                                       OrderBookType OrderBookType);

    private:
    static OrderBookEntry stringToOBE(vector<string> strings);



    
};

