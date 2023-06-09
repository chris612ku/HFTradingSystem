#include "MdSpi.h"
#include "Constants.h"
#include "KData.h"
#include "mystruct.h"
#include "Strategy.h"
#include "TdSpi.h"

#include <assert.h>
#include <iostream>
#include <unordered_map>
#include <mutex>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

using namespace std;

extern unordered_map<string, string> accountConfig_map;

extern mutex m_mutex;

extern TdSpi* g_pUserTDSpi_AI;

extern Strategy* g_strategy;

sql::Driver* driver;

sql::Connection* con;

sql::Statement* stmt;

sql::ResultSet* res;

MdSpi::MdSpi(CThostFtdcMdApi *mdApi) : mdApi(mdApi), nRequestID(0)
{
	assert(mdApi);
	m_BrokerId = accountConfig_map[accountConfigKey_brokerId];
	m_UserId = accountConfig_map[accountConfigKey_userId];
	m_Passwd = accountConfig_map[accountConfigKey_passwd];
	strcpy_s(m_InstId, accountConfig_map[accountConfigKey_contract].c_str());

	m_KData = new CKDataCollection();
}

MdSpi::~MdSpi()
{
	delete[] m_InstIDList_all;
	delete[] m_InstIDList_Position_MD;
	delete loginField;

	callbacks.clear();
}

void MdSpi::OnFrontConnected()
{
	cerr << "[MdSpi]: OnFrontConnected invoked" << endl;
	ReqUserLogin(m_BrokerId, m_UserId, m_Passwd);
}

void MdSpi::ReqUserLogin(std::string brokerID, std::string userID, std::string password)
{
	loginField = new CThostFtdcReqUserLoginField(); 
	strcpy_s(loginField->BrokerID, brokerID.c_str()); 
	strcpy_s(loginField->UserID, userID.c_str()); 
	strcpy_s(loginField->Password, password.c_str()); 
	int nResult = mdApi->ReqUserLogin(loginField, GetNextRequestId()); 
	
	cerr << "[MdSpi]: Request user login " << ((nResult == 0) ? "successfully" : "with failure") << endl;
}

void MdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[MdSpi]: Received response for user login" << endl;
	if (IsErrorRspInfo(pRspInfo))
	{
		cerr << "[MdSpi]: Failed to login user due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << "]" << endl;
		return;
	}

	if (!pRspUserLogin)
	{
		cerr << "[MdSpi]: Null pRspUserLogin callback parameter" << endl;
		return;
	}

	string tradingDay = mdApi->GetTradingDay();
	cerr << "[MdSpi]: User login successfully" << endl;
	cerr << "[MdSpi]: Trading Day: " << tradingDay << endl;

	CreateKBarDirectory();

	// 生成保存tick数据的文件夹
	//directory = HFTradingSystemDirectoryPrefix + "database";
	//Mkdir(directory); // 首先生成C:/Users\user/Desktop/HFTradingSystem/HFTradingSystem/database

	//directory += "/" + tradingDay;
	//int nRet = Mkdir(directory);
	//if (!nRet)
	//{
	//	printf("[TdSpi]: Successfully created directory %s\n", directory.c_str());
	//}
	//else
	//{
	//	printf("[TdSpi]: Failed to create directory %s\n", directory.c_str());
	//}

	cerr << "[MdSpi]: Trying to subscribe market data" << endl;

	// 插入合约到订阅列表
	bool exist = false;
	for (char* instr : subscribe_inst_vec)
	{
		if (!strcmp(instr, m_InstId))
		{
			cerr << "[MdSpi]: The strategy instrument is already in the subscribed vector" << endl;
			exist = true;
			break;
		}
	}

	if (!exist && strlen(m_InstId))
	{
		addInstrToSubscrbedInstrVec(m_InstId);
	}

	int count = subscribe_inst_vec.size();
	if (!count)
	{
		cerr << "[MdSpi]: No instrument needs to be subscribed for market data" << endl;
		return;
	}

	// 生成保存tick数据的文件，每个合约一个文件
	/*for (char* instID : subscribe_inst_vec)
	{
		CreateFile(directory + "/", instID);
	}*/

	try {
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "Taipei9536");
		/* Connect to the MySQL test database */
		con->setSchema(marketdataSchema);
		stmt = con->createStatement();
		/*stmt->execute("DROP TABLE IF EXISTS test");
		stmt->execute("CREATE TABLE test(id INT)");
		delete stmt;*/
	} catch (sql::SQLException& e) {
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line "
			<< __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << 
			" )" << endl;
	}

	auto it = g_pUserTDSpi_AI->m_inst_field_map.find(m_InstId);
	auto endIt = g_pUserTDSpi_AI->m_inst_field_map.end();
	if (it != endIt)
	{
		int contractSize = it->second->VolumeMultiple;
		m_KData->CreateSeries(m_InstId, Minute, 1, contractSize, 2000);

		const CKBarSeries* series = m_KData->GetSeries(m_InstId, Minute, 1);
		if (series)
		{
			string kbarfilename = KbarDirectory + series->m_sDisplayName;
			CreateKBarDataFile(kbarfilename);
		}
	}

	cerr << "[MdSpi]: Subscribing market data for " << count << " instrument(s)" << endl;
	SubcribeMarketData();

	g_strategy->OnStrategyStart();
	//策略启动后默认禁止开仓
	cerr << endl << endl << endl << "[MdSpi]: The strategy by default forbids opening a position, if we want to open a position, please input the command (allow: yxkc, disallow: jzkc)" << endl;
}

void MdSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void MdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[MdSpi]: Received response for subscribing market data" << endl;
	if (IsErrorRspInfo(pRspInfo))
	{
		cerr << "[MdSpi]: Failed to subscribe market data due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << "]" << endl;
		return;
	}

	if (!pSpecificInstrument)
	{
		cerr << "[MdSpi]: Null pSpecificInstrument callback parameter" << endl;
		return;
	}
}

void MdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void MdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	for (auto it : callbacks)
	{
		it.second(pDepthMarketData);
	}

	if (!strcmp(pDepthMarketData->InstrumentID, m_InstId))
	{
		cerr << "[MdSpi]: InstrumentID: " << pDepthMarketData->InstrumentID << ", LastPrice: " << pDepthMarketData->LastPrice
			<< ", UpdateTime: " << pDepthMarketData->UpdateTime << ", UpdateMillisec: " << pDepthMarketData->UpdateMillisec << endl;
		m_KData->AddTickData(pDepthMarketData);

	}
	
	//g_strategy->OnTick(pDepthMarketData);
	// 保存tick数据到csv文件
	//SaveTick(directory + "/", pDepthMarketData);
}

void MdSpi::SubscribeMarketData(char* instIdList)
{
	//char instIdList[]="IF2012,IF2101,IF2103"
	vector<char*>list;
	/* strtok()用来将字符串分割成一个个片段。参数s指向欲分割的字符串，参数delim则为分割字符串中包含的所有字符。
	   当strtok()在参数s的字符串中发现参数delim中包含的分割字符时,则会将该字符改为\0字符。
	   在第一次调用时，strtok()必需给予参数s字符串，往后的调用则将参数s设置成NULL。每次调用成功则返回指向被分割出片段的指针。
	*/
	char *token = strtok(instIdList, ","); 
	while (token != NULL) 
	{ 
		list.push_back(token); 
		token = strtok(NULL, ","); 
	}
	unsigned int len = list.size(); 
	char **ppInstrument = new char*[len]; 
	for (unsigned int i = 0; i < len; ++i)
	{
		ppInstrument[i] = list[i]; //指针赋值，没有新分配内存空间
	}
	int nRet = mdApi->SubscribeMarketData(ppInstrument, len);
	cerr << "[MdSpi]: Subscribe market data for " << instIdList << " " << ((nRet == 0) ? "successfully" : "with failure") << endl;
		
	delete[] ppInstrument;
}

void MdSpi::SubscribeMarketData(std::string instIdList)
{
	SubscribeMarketData(instIdList.c_str());
}

void MdSpi::SubcribeMarketData_All()
{
	SubscribeMarketData(m_InstIDList_all);
}

void MdSpi::SubcribeMarketData()
{
	int count = subscribe_inst_vec.size();
	char** subscribeInstruments = new char*[count];
	for (int i = 0; i < count; ++i)
	{
		subscribeInstruments[i] = subscribe_inst_vec[i];
	}

	int nResult = mdApi->SubscribeMarketData(subscribeInstruments, count);
	cerr << "[MdSpi]: Subscribe market data for instrument(s) " << ((nResult == 0) ? "successfully" : "with failure") << endl;

	delete[] subscribeInstruments;
}

void MdSpi::setInstIDList_Position_MD(std::string& inst_holding)
{
	m_InstIDList_Position_MD = new char[inst_holding.length()];
	memcpy(m_InstIDList_Position_MD, inst_holding.c_str(), sizeof(inst_holding.length()));
}

void MdSpi::setInstIDListAll(std::string& inst_all)
{
	int len = inst_all.length();
	if (!len)
	{
		cerr << "[MdSpi]: Failed to set instrument list all due to empty string" << endl;
		return;
	}

	m_InstIDList_all = new char[len + 1];
	strcpy(m_InstIDList_all, inst_all.c_str());
	m_InstIDList_all[len] = '\0';
}

bool MdSpi::hasSubscribedInstr(char* instr)
{
	for (char* subscribedInstr : subscribe_inst_vec)
	{
		if (!strcmp(subscribedInstr, instr))
		{
			return true;
		}
	}

	return false;
}

std::vector<char *> MdSpi::getSubscribedInstrVec()
{
	return subscribe_inst_vec;
}

void MdSpi::addInstrToSubscrbedInstrVec(char *newInstr)
{
	int len = strlen(newInstr);
	if (!len)
	{
		cerr << "[MdSpi]: Invalid newInstru with length 0" << endl;
		return;
	}

	char *instr = new char[len];
	strcpy(instr, newInstr);
	subscribe_inst_vec.push_back(instr);
}

int MdSpi::addObserverCallback(std::function<void(CThostFtdcDepthMarketDataField*)> callback)
{
	int observerID = GetNextObserverID();
	callbacks[observerID] = callback;
	return observerID;
}

void MdSpi::removeObserverCallback(int observerID)
{
	callbacks.erase(observerID);
}

bool MdSpi::hasRegisteredForCallback(int observerID)
{
	return callbacks.find(observerID) != callbacks.end();
}

int MdSpi::GetNextRequestId()
{
	lock_guard<mutex> lock(m_mutex);
	return ++nRequestID;
}

int MdSpi::GetNextObserverID()
{
	lock_guard<mutex> lock(m_mutex);
	return ++observerID;
}

bool MdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	return pRspInfo && pRspInfo->ErrorID != 0;;
}
