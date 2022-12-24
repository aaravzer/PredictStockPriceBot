#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <limits>

#include <curl/curl.h>
#include <json/json.h>

using namespace std;

// Output file name
const string OUTPUT_FILE = "stockData.csv";

// Stock data structure
struct StockData {
	string symbol;
	vector<double> prices;
	vector<double> returns;
};

// Curl write data callback function
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

// Function to get stock data from the web
StockData getStockData(const vector<string>& symbols) {
	StockData stockData;
	
	// Set up Curl
	CURL *curl;
	CURLcode res;
	string readBuffer;
	
	// Go through each symbol
	for(string symbol : symbols) {
		// Set up Curl
		curl = curl_easy_init();
		if(curl) {
			// Set URL
			string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=" + symbol + "&apikey=your_api_key";
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			
			// Set write callback
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			
			// Perform request
			res = curl_easy_perform(curl);
			
			// Check for errors
			if(res != CURLE_OK) {
				cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
				exit(1);
			}
			
			// Cleanup
			curl_easy_cleanup(curl);
			
			// Parse JSON
			Json::Value root;
			Json::Reader reader;
			bool parsingSuccessful = reader.parse(readBuffer.c_str(), root);
			
			// Check for errors
			if(!parsingSuccessful) {
				cerr << "Failed to parse JSON" << endl;
				exit(1);
			}
			
			// Get data
			Json::Value dailyData = root["Time Series (Daily)"];
			for(Json::ValueIterator iter = dailyData.begin(); iter != dailyData.end(); iter++) {
				Json::Value data = *iter;
				string date = iter.key().asString();
				double close = data["4. close"].asDouble();
				
				stockData.symbol = symbol;
				stockData.prices.push_back(close);
			}
			
			// Calculate returns
			for(int i = 1; i < stockData.prices.size(); i++) {
				double returnVal = (stockData.prices[i] - stockData.prices[i - 1]) / stockData.prices[i - 1];
				stockData.returns.push_back(returnVal);
			}
		}
	}
	
	return stockData;
}

// Function to calculate linear regression
pair<double, double> linearRegression(const vector<double>& x, const vector<double>& y) {
	// Check for valid input
	if(x.size() != y.size()) {
		cerr << "Input vectors have different sizes!" << endl;
		exit(1);
	}
	
	// Calculate mean values
	double meanX = 0.0;
	double meanY = 0.0;
	for(int i = 0; i < x.size(); i++) {
		meanX += x[i];
		meanY += y[i];
	}
	meanX /= x.size();
	meanY /= y.size();
	
	// Calculate coefficients
	double numerator = 0.0;
	double denominator = 0.0;
	for(int i = 0; i < x.size(); i++) {
		numerator += (x[i] - meanX) * (y[i] - meanY);
		denominator += (x[i] - meanX) * (x[i] - meanX);
	}
	
	double beta = numerator / denominator;
	double alpha = meanY - beta * meanX;
	
	return make_pair(alpha, beta);
}

// Function to predict stock prices
double predictStockPrice(const StockData& data, int days) {
	// Get linear regression coefficients
	pair<double, double> coefficients = linearRegression(data.returns, data.returns);
	double alpha = coefficients.first;
	double beta = coefficients.second;
	
	// Calculate predicted return
	double predictedReturn = alpha + beta * days;
	
	// Calculate predicted price
	double predictedPrice = data.prices.back() * exp(predictedReturn);
	
	return predictedPrice;
}
