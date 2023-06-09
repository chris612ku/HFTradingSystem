#include "main.h"
#include "Constants.h"
#include "KData.h"
#include "MdSpi.h"
#include "TdSpi.h"
#include "Strategy.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <thread>

using namespace std;

// 上个交易日
char g_cPreTradingDay[9];

// 下个自然日
char g_cNextTradeDate[9];

//保存账户信息的map
unordered_map<string, string> accountConfig_map;

//策略类指针
Strategy *g_strategy;

//全局的TD回调处理类对象，AI交互函数会用到
TdSpi *g_pUserTDSpi_AI;

//AI线程函数
void AIThread();

void ParseConfig();

//请求编号
int requestId;

// 全局的互斥锁
std::mutex m_mutex;

int main()
{
	cerr << "---------------------------------------------" << endl;
	cerr << "---------------------------------------------" << endl;
	cerr << "----High Frequency Trading System Started----" << endl;
	cerr << "---------------------------------------------" << endl;
	cerr << "---------------------------------------------" << endl;

	g_cPreTradingDay[0] = '\0';
	g_cNextTradeDate[0] = '\0';

	cout << "Please enter the last trading date (e.g. 20230510): " << endl;

	cin >> g_cPreTradingDay;
	fflush(stdin);
	long ltd = GetNextDate(atol(g_cNextTradeDate));
	sprintf(g_cNextTradeDate, "%ld", ltd);

	ReadTradeTimeMap();

	// 1. 读取账户信息
	ParseConfig();

	// 2. 创建行情api和回调
	CThostFtdcMdApi* pUserMarketApi = CThostFtdcMdApi::CreateFtdcMdApi("./Temp/Marketflow/");
	MdSpi* pUserMarketSpi = new MdSpi(pUserMarketApi);
	pUserMarketApi->RegisterSpi(pUserMarketSpi); // api注册回调类

	char mdFront[50];
	strcpy_s(mdFront, accountConfig_map["MarketFront"].c_str());
	pUserMarketApi->RegisterFront(mdFront);

	// 3. 创建交易api和回调
	CThostFtdcTraderApi* pUserTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("./Temp/Tradeflow/");
	TdSpi* pUserTraderSpi = new TdSpi(pUserTraderApi, pUserMarketApi, pUserMarketSpi);
	pUserTraderApi->RegisterSpi(pUserTraderSpi); // api注册回调类

	pUserTraderApi->SubscribePublicTopic(THOST_TERT_RESTART); // 订阅公有流
	pUserTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK); // 订阅私有流

	char tdFront[50];
	strcpy_s(tdFront, accountConfig_map["TradeFront"].c_str());
	pUserTraderApi->RegisterFront(tdFront);

	// 4. 创建策略
	g_strategy = new Strategy(pUserTraderSpi);

	// 5. 启动交易的线程
	pUserTraderApi->Init(); // api提供的启动线程

	// 6. 启动AI线程
	g_pUserTDSpi_AI = pUserTraderSpi;
	std::thread aiThread(AIThread);

	// 7. 等待线程退出
	pUserMarketApi->Join();
	pUserTraderApi->Join();
	aiThread.join();

	pUserMarketApi->Release(); // can write in desctructor
	pUserTraderApi->Release(); // can write in desctructor
	return 0;
}

void AIThread()
{

}

void ParseConfig()
{
	string key, value;
	char dataLine[256];
	ifstream fin(".\\config\\config.txt", ios::in);
	
	if (!fin)
	{
		cerr << "config文件不存在" << endl;
		return;
	}

	while (fin.getline(dataLine, sizeof(dataLine), newLine))
	{
		int length = strlen(dataLine);
		string temp;
		for (int i = 0, count = 0; i < length + 1; ++i)
		{
			if (dataLine[i] != comma && dataLine[i] != endCharacter)
			{
				temp += dataLine[i];
				continue;
			}
		
			switch (count)
			{
				case 0:
					key = temp;
					break;
				case 1:
					value = temp;
					break;
			}

			temp = "";
			++count;
		}

		accountConfig_map[key] = value;
	}
}
