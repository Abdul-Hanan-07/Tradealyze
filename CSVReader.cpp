#include "CSVReader.h"
#include <iostream>
#include <fstream>

CSVReader::CSVReader()
{

}

vector<OrderBookEntry> CSVReader::readCSV(string csvFilename)
{
    vector<OrderBookEntry> entries;
    ifstream csvFile{csvFilename};
    string line;
    
    // 👇 NEW: Trackers to find the exact broken lines
    int lineNum = 0;
    int badLines = 0;

    if(csvFile.is_open())
    {
        while(getline(csvFile, line))
        {   
            lineNum++; // Increment line counter
            try{
                OrderBookEntry obe = stringToOBE(tokenise(line, ','));
                entries.push_back(obe);
            }catch(exception& e)
            {
                badLines++; // Increment bad line counter
                // 👇 NEW: Print the exact line number and the corrupted text
                cout << "Bad Data on line " << lineNum << ": [" << line << "]" << endl;
            }
        } //end of while
    }
   
    // 👇 NEW: Update the final print to show how many were skipped
    cout << "CSVReader::readCSV loaded " << entries.size() << " entries, skipped " 
         << badLines << " bad lines." << endl;
         
    return entries;
}

vector<string> CSVReader::tokenise(string csvline, char separator)
{
    vector<string> tokens;
    signed int start, end;
    string token;

    start = csvline.find_first_not_of(separator, 0);

    do{
        end = csvline.find_first_of(separator, start);
        if(start == csvline.length() || start == end) break;
        if(end >= 0) token = csvline.substr( start, end-start);
        else token = csvline.substr(start, csvline.length() - start);
        tokens.push_back(token);
        start = end + 1;
    }while(end > 0);

    return tokens;
}

OrderBookEntry CSVReader::stringToOBE(vector<string> tokens)
{
    double price, amount;

    if(tokens.size() != 5) // Bad Result
    {
        cout << "Bad Line" << endl;
        throw exception{};
    }

    // we have 5 tokens
    try{
        price = stod(tokens[3]);
        amount = stod(tokens[4]);
         
    // If the data is invalid so it will catch the exception
    }catch(const exception& e){
        // cout<<"CSVReader::stringToOBE Bad float! "<< tokens[3]<<endl;
        // cout<<"CSVReader::stringToOBE Bad Float! "<< tokens[4]<<endl;
        throw;
    }
          
    OrderBookEntry obe{price,
                       amount,  
                       tokens[0],
                       tokens[1],
                       OrderBookEntry::stringToOrderBookType(tokens[2])};

    return obe;              
}

OrderBookEntry CSVReader::stringToOBE (string priceString,
                                    string amountString, 
                                    string timestamp, 
                                    string product,
                                    OrderBookType orderType)
{        
    double price, amount;
    try{
        price = stod(priceString);
        amount = stod(amountString);
   
    // If the data is invalid so it will catch the exception
    }catch(const exception& e){
        cout << "CSVReader::stringToOBE Bad float! " << priceString << endl;
        cout << "CSVReader::stringToOBE Bad Float! " << amountString << endl;
        throw;
    }

    OrderBookEntry obe{price,
                       amount,       
                       timestamp,
                       product,
                       orderType};
      
    return obe;
}