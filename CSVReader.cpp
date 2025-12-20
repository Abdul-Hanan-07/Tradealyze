#include"CSVReader.h"
#include<iostream>
#include<fstream>

CSVReader::CSVReader()
{

}
vector<OrderBookEntry> CSVReader::readCSV(string csvFilename)
{
   vector<OrderBookEntry> entries;

    ifstream csvFile{csvFilename};
    string line;
    if(csvFile.is_open())
    {
      while(getline(csvFile, line))
      {   
         try{
            OrderBookEntry obe = stringToOBE(tokenise(line, ','));
          entries.push_back(obe);
         }catch(exception& e)
         {
             cout<<"Bad Data Csv reader"<<endl;
         }
          
      } //end of while
   }
   
   cout<<"CSVReader::readCSV read "<<entries.size()<<"entries"<<endl;
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
   if(start== csvline.length() || start == end) break;
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
                cout<<"Bad Line"<<endl;
                throw exception{};
             }

            // we have 5 tokens
            try{
                price = stod(tokens[3]);
                amount = stod(tokens[4]);
         
         // If the data is invalid so it will catch the exception
            }catch(const exception& e){
                cout<<"CSVReader::stringToOBE Bad float!"<< tokens[3]<<endl;
                cout<<"CSVReader::stringToOBE Bad Float!"<< tokens[4]<<endl;
                
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
                                    OrderBookType orderType
                                     )
{         
   double price, amount;
   try{
            price = stod(priceString);
            amount = stod(amountString);
   
   // If the data is invalid so it will catch the exception
      }catch(const exception& e){
            cout<<"CSVReader::stringToOBE Bad float!"<< priceString<<endl;
            cout<<"CSVReader::stringToOBE Bad Float!"<< amountString<<endl;
            throw;
      }

           OrderBookEntry obe{price,
                   amount,       
                   timestamp,
                   product,
                   orderType};
      
  return obe;

}                                    
