#include "mystruct.h"
#include "direct.h"
#include "io.h"
#include "KData.h"
#include "ThostTraderApi/ThostFtdcUserApiStruct.h"

#include <fstream>
#include <iostream>

using namespace std;

void ReadKbarSeries(string fileName, vector<Kbar>& kbar_vec)
{
}

void Save_FileName(string path, vector<string>& fileName_vec)
{
}

int Mkdir(string& directory)
{
	const char* directoryCString = directory.c_str();
	if (!_access(directoryCString, 0))
	{
		return -1;
	}

	return _mkdir(directoryCString);
}

void SaveTick(string filePath, CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	if (!pDepthMarketData)
	{
		return;
	}

	char filePathCString[100] = { '\0' };
	sprintf_s(filePathCString, sizeof(filePathCString), "%s%s_market_data.csv", filePath.c_str(), pDepthMarketData->InstrumentID);

	ofstream outFile;
	outFile.open(filePathCString, ios::app);
	outFile << pDepthMarketData->InstrumentID << "," << pDepthMarketData->UpdateTime << "." << pDepthMarketData->UpdateMillisec << "," 
			<< pDepthMarketData->LastPrice << "," << pDepthMarketData->Volume << "," << pDepthMarketData->BidPrice1 << "," 
			<< pDepthMarketData->BidVolume1 << "," << pDepthMarketData->AskPrice1 << "," << pDepthMarketData->AskVolume1 << "," 
			<< pDepthMarketData->OpenInterest << "," << pDepthMarketData->Turnover << std::endl; outFile.close();
}

int CreateFile(string directory, char* cInst)
{
	char filePath[150] = { '\0' };
	sprintf_s(filePath, sizeof(filePath), "%s%s_market_data.csv", directory.c_str(), cInst);
	
	ofstream outFile;
	if (!_access(filePath, 0))
	{
		return -1;
	}

	outFile.open(filePath, ios::out); 
	outFile << "InstrumentID" << "," << "UpdateTime" << "," << "LastPrice" << "," << "TradeVolume" << "," 
			<< "BidPrice1" << "," << "BidVolume1" << "," << "AskPrice1" << "," << "AskVolume1" << "," 
			<< "OpenInterest" << "," << "Turnover" << "," << endl; 
	
	outFile.close();
}

void CreateKBarDirectory()
{
	string directory = HFTradingSystemDirectoryPrefix;
	int nRet = Mkdir(directory);

	directory = KbarDirectory;
	nRet = Mkdir(directory);
	if (!nRet)
	{
		cerr << "KBar directory was created successfully" << endl;
	}
	else
	{
		cerr << "Failed to create KBar directory" << endl;
	}
}

void CreateKBarDataFile(std::string& filename)
{
	char filenameCString[200] = { '\0' };
	strcpy_s(filenameCString, filename.c_str());
	sprintf_s(filenameCString, sizeof(filenameCString), "%s.csv", filenameCString);
	
	ofstream outFile;
	if (!_access(filenameCString, 0))
	{
		return;
	}

	outFile.open(filenameCString, ios::out);
	outFile << "InstrumentID" << "," << "ActionDate" << "," << "TradingDate" << "," << "BeginTime" << ","
		<< "EndTime" << "," << "OpenPrice" << "," << "HighestPrice" << "," << "LowestPrice" << ","
		<< "ClosePrice" << "," << "AveragePrice" << "," << "Volume" << "," << "Position" << ","
		<< "PositionDiff" << "," << endl;
	outFile.close();
}
