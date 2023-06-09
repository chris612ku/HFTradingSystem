#pragma once
#include "ThostTraderApi/ThostFtdcMdApi.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class CKDataCollection;

class MdSpi : public CThostFtdcMdSpi
{
public:
	MdSpi(CThostFtdcMdApi *mdApi);

	~MdSpi();

	// 建立连接时触发
	void OnFrontConnected();

	// 请求登录
	void ReqUserLogin(std::string brokerID, std::string userID, std::string password);

	// 登录请求响应
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// 登出请求响应
	void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// 订阅行情应答
	void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// 取消订阅行情应答
	void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// 深度行情通知
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData);

	void SubscribeMarketData(char *instIdList);

	void SubscribeMarketData(std::string instIdList);

	// 订阅所有的合约
	void SubcribeMarketData_All();

	void SubcribeMarketData();

	void setInstIDList_Position_MD(std::string &inst_holding);

	void setInstIDListAll(std::string& inst_all);

	bool hasSubscribedInstr(char* instr);

	std::vector<char *> getSubscribedInstrVec();

	void addInstrToSubscrbedInstrVec(char *newInstr);

	int addObserverCallback(std::function<void(CThostFtdcDepthMarketDataField *)> callback);

	void removeObserverCallback(int observerID);

	bool hasRegisteredForCallback(int observerID);

private:
	CThostFtdcMdApi* mdApi;

	CKDataCollection* m_KData;

	CThostFtdcReqUserLoginField* loginField;

	std::string m_BrokerId;

	std::string m_UserId;

	std::string m_Passwd;

	std::string directory;

	// 策略里面需要交易的合约
	char m_InstId[32];

	// 持仓的合约
	char* m_InstIDList_Position_MD;

	// 所有的合约
	char* m_InstIDList_all;

	// 所有订阅合约
	std::vector<char *> subscribe_inst_vec;

	std::unordered_map<int, std::function<void(CThostFtdcDepthMarketDataField*)>> callbacks;

	int nRequestID;

	int observerID;

	int GetNextRequestId();

	int GetNextObserverID();

	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
};