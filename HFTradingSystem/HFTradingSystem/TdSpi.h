#pragma once
#include <functional>
#include <list>
#include <unordered_map>
#include <set>
#include <vector>

#include "ThostTraderApi/ThostFtdcTraderApi.h"
#include "MdSpi.h"

struct position_field;

class TdSpi : public CThostFtdcTraderSpi
{ 
public:
	// 构造函数
	TdSpi(CThostFtdcTraderApi *tdApi, CThostFtdcMdApi *pUserApi_md, MdSpi *pUserSpi_md);

	~TdSpi();
	
	// 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	void OnFrontConnected();

	// 当客户端与交易后台无法建立起通信连接时（还未登录前），该方法被调用。
	void OnFrontDisconnected(int nReason);
	
	// 请求用户登录
	int ReqUserLogin();
	
	// 客户端认证请求
	int ReqAuthenticate();
	
	// 请求查询订单
	void ReqQryOrder();
	
	// 请求查询成交
	void ReqQryTrade();
	
	// 请求查询持仓明细
	void ReqQryInvestorPositionDetail();
	
	void ReqQryTradingAccount();

	void ReqQryInvestorPositionAll();
	
	void ReqQryInvestorPosition(std::string instrument);
	
	void ReqQryInstrumetAll();
	
	void ReqQryInstrumet(std::string instrument);

	void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	// 是否出现错误信息
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);

	// 登录请求响应
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	// 登出请求响应 
	void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	// 请求确认结算单
	void ReqSettlementInfoConfirm();
	
	// 请求查询结算信息确认响应
	void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID ,bool bIsLast) override;
	
	// 请求查询投资者结算结果响应
	void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 投资者结算结果确认响应
	void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 用户口令更新请求响应
	void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	// 请求查询行情响应
	void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	// 请求查询投资者持仓响应
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 请求查询投资者持仓响应
	void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)override;
	
	// 请求查询成交响应
	void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 请求查询成交响应
	void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 请求查询持仓明细响应
	void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 查询资金帐户响应
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount ,CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	
	// 报单通知
	void OnRtnOrder(CThostFtdcOrderField*pOrder);
	
	// 成交通知
	void OnRtnTrade(CThostFtdcTradeField*pTrade);

	// 报单录入请求响应
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// 报单录入错误回报
	void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo);

	// 报单操作请求响应
	void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// 报单操作错误回报
	void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo);
	
	// make an order;
	CThostFtdcOrderField* PlaceOrder(const char*pszCode, const char *ExchangeID, const char nDirection, int nOpenClose, int nVolume, double fPrice);
	
	// cancel an order;
	void CancelOrder(CThostFtdcOrderField*pOrder);
	
	// 打印持仓信息
	void ShowPosition();
	
	// 平掉账户的仓位
	void ClosePosition();
	
	// 设置程序化策略能否开放
	void SetAllowOpen(bool isOk);
	
	// 更新订单状态
	bool UpdateOrder(CThostFtdcOrderField *pOrder);

	bool UpdateOrder(CThostFtdcOrderField* pOrder, std::unordered_map<int, CThostFtdcOrderField*> &orders);

	// 用于保存所有合约映射
	std::unordered_map<std::string, CThostFtdcInstrumentField*> m_inst_field_map;

	std::unordered_map<std::string, position_field*> m_position_field_map;

	CThostFtdcOrderField* GetOrder(CThostFtdcOrderField* pOrder);

private:
	CThostFtdcTraderApi *m_pUserTDApi_trade;

	CThostFtdcMdApi *m_pUserMDApi_trade;

	MdSpi *m_pUserMDSpi_trade;

	CThostFtdcReqUserLoginField *loginField;
	
	CThostFtdcReqAuthenticateField *authField;
	
	int nRequestID;

	int observerID;
	
	// 当天的所有报单
	std::unordered_map<int, CThostFtdcOrderField *> orderMap;
	
	// 当天的所有成交
	std::vector<CThostFtdcTradeField *> tradeList;
	
	// 未平仓的多单持仓
	std::list<CThostFtdcTradeField *> positionDetailList_NotClosed_Long;
	
	// 为平仓的空单持仓
	std::list<CThostFtdcTradeField *> positionDetailList_NotClosed_Short;
	
	// 本程序的成交ID集合
	std::set<std::string> m_TradeIDs;

	// 本程序的成交vector
	std::vector< CThostFtdcTradeField *> m_Trades;
	
	//本程序的报单集合
	std::unordered_map<int, CThostFtdcOrderField *> m_Orders;
	
	//map<int, CThostFtdcOrderField *> m_CancelingOrders;

	// 当天所有报单，frontID + sessionID + orderRef
	std::unordered_map<std::string, CThostFtdcOrderField*> m_mOrders;
	
	double m_CloseProfit;
	
	double m_OpenProfit;

	double m_PositionProfit;
	
	bool m_QryOrder_Once;
	
	bool m_QryTrade_Once;
	
	bool m_QryDetail_Once;
	
	bool m_QryTradingAccount_Once;
	
	bool m_QryPosition_Once;
	
	bool m_QryInstrument_Once;
	
	std::string m_AppId;
	
	std::string m_AuthCode;

	std::string m_BrokerId;
	
	std::string m_UserId;
	
	std::string m_Passwd;
	
	std::string m_InstId;
	
	std::string m_Instrument_All;
	
	int m_nFrontID;
	
	int m_nSessionID;
	
	bool m_AllowOpen;

	char orderRef[13];
	
	TThostFtdcDateType m_cTradingDay;

	// 等待三秒并执行下一个查询
	void waitAndPerformNextQuery(std::function<void()> queryFunc);

	int GetNextRequestId();

	char* GetNextOrderRef();

	// 通过brokerOrderSeq获取报单
	CThostFtdcOrderField* GetOrder(int nBrokerOrderSeq);

	void ShowInstMessage();

	void ShowOrders(std::unordered_map<int, CThostFtdcOrderField*> orders);

	void ShowTradeList(std::vector<CThostFtdcTradeField*> trades);

	void ShowPositionDetail();

	CThostFtdcTradeField *GetTrade(CThostFtdcTradeField * pTrade, std::vector<CThostFtdcTradeField*> trades);

	std::unordered_map<int, CThostFtdcOrderField *> GetOrderMap();

	std::unordered_map<int, CThostFtdcOrderField*> GetmOrders();

	void addOrderToOrderMap(CThostFtdcOrderField* pOrder);

	std::vector<CThostFtdcTradeField *> GetTradeList();

	std::vector<CThostFtdcTradeField*> GetmTrades();

	std::list<CThostFtdcTradeField*> GetPositionDetailList_NotClosed_Long();

	std::list<CThostFtdcTradeField*> GetPositionDetailList_NotClosed_Short();

	void addTradeToList(CThostFtdcTradeField *pTrade, std::vector<CThostFtdcTradeField*> &trades);

	bool updatePositionDetail(CThostFtdcTradeField* pTrade);

	void updatePositionFieldMap(TThostFtdcInstrumentIDType instrumentID);

	void updatePosition(std::string instrumentID);

	void updatePositionMDPrice(CThostFtdcDepthMarketDataField *pDepthMarketData);

	void refreshPositionField(std::string instrumentID, position_field* positionField);

	bool isMDPriceUpdated();
};