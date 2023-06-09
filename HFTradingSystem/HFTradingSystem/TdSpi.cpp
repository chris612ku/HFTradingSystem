#include "TdSpi.h"
#include "Constants.h"
#include "mystruct.h"
#include "Strategy.h"

#include <assert.h>
#include <iostream>
#include <mutex>

using namespace std;

extern unordered_map<string, string> accountConfig_map;

extern mutex m_mutex;

extern Strategy *g_strategy;

typedef void(TdSpi::* queryFunc)();

TdSpi::TdSpi(CThostFtdcTraderApi *tdApi, CThostFtdcMdApi *pUserApi_md, MdSpi *pUserSpi_md) : m_pUserTDApi_trade(tdApi), m_pUserMDApi_trade(pUserApi_md), m_pUserMDSpi_trade(pUserSpi_md), nRequestID(0),
																							 m_QryOrder_Once(true), m_QryTrade_Once(true), m_QryDetail_Once(true), m_QryTradingAccount_Once(true),
																							 m_QryPosition_Once(true), m_QryInstrument_Once(true), observerID(0)
{
	/*assert(tdApi);
	assert(pUserApi_md);
	assert(pUserSpi_md);*/

	// appid, authcode, brokerid, userid, passwd
	m_AppId = accountConfig_map[accountConfigKey_appId];
	m_AuthCode = accountConfig_map[accountConfigKey_authcode];
	m_BrokerId = accountConfig_map[accountConfigKey_brokerId];
	m_UserId = accountConfig_map[accountConfigKey_userId];
	m_Passwd = accountConfig_map[accountConfigKey_passwd];
}

TdSpi::~TdSpi()
{
	for (int i = 0; i < tradeList.size(); ++i)
	{
		delete tradeList[i];
	}
	
	tradeList.clear();

	for (auto& it : orderMap)
	{
		delete it.second;
	}

	orderMap.clear();

	for (auto it = positionDetailList_NotClosed_Long.begin(); it != positionDetailList_NotClosed_Long.end(); ++it)
	{
		delete (*it);
	}

	positionDetailList_NotClosed_Long.clear();

	for (auto it = positionDetailList_NotClosed_Short.begin(); it != positionDetailList_NotClosed_Short.end(); ++it)
	{
		delete (*it);
	}

	positionDetailList_NotClosed_Short.clear();

	for (auto it = m_position_field_map.begin(); it != m_position_field_map.end(); ++it)
	{
		delete it->second;
	}

	m_position_field_map.clear();

	for (auto it = m_Orders.begin(); it != m_Orders.end(); ++it)
	{
		delete it->second;
	}

	m_Orders.clear();

	for (auto it = m_Trades.begin(); it != m_Trades.end(); ++it)
	{
		delete (*it);
	}

	m_Trades.clear();

	m_pUserMDSpi_trade->removeObserverCallback(observerID);

	for (auto it = m_mOrders.begin(); it != m_mOrders.end(); ++it)
	{
		delete it->second;
	}
}

void TdSpi::OnFrontConnected()
{
	cerr << "[TdSpi]: OnFrontConnected invoked" << endl;

	static const char* version = m_pUserTDApi_trade->GetApiVersion();
	cerr << "[TdSpi]: Current CTP Api Version: " << version << endl;

	ReqAuthenticate();
}

void TdSpi::OnFrontDisconnected(int nReason)
{
	cerr << "[TdSpi]: OnFrontDisconnected invoked due to reason: " << nReason << endl;
}

int TdSpi::ReqUserLogin()
{
	cerr << "[TdSpi]: Requesting user login..." << endl;
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.UserID, m_UserId.c_str());
	strcpy_s(req.Password, m_Passwd.c_str());

	cerr << "[TdSpi]: Requesting user login info: " << endl << "\t[brokerId]: " << req.BrokerID << endl << "\t[userId]: " << req.UserID << endl << "\t[passwd]: " << req.Password << endl;

	return m_pUserTDApi_trade->ReqUserLogin(&req, GetNextRequestId());
}

int TdSpi::ReqAuthenticate()
{
	cerr << "[TdSpi]: Requesting authenticate" << endl;

	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.AppID, m_AppId.c_str());
	strcpy_s(req.AuthCode, m_AuthCode.c_str());
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.UserID, m_UserId.c_str());

	cerr << "[TdSpi]: Requesting authenticate info: " << endl << "\t[appid]: " << req.AppID << endl << "\t[authcode]: " << req.AuthCode << endl 
		<< "\t[brokerId]: " << req.BrokerID << endl << "\t[userId]: " << req.UserID << endl;

	return m_pUserTDApi_trade->ReqAuthenticate(&req, GetNextRequestId());;
}

void TdSpi::ReqQryOrder()
{
	cerr << "[TdSpi]: Requesting order query" << endl;

	CThostFtdcQryOrderField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.InvestorID, m_UserId.c_str());
	
	cerr << "[TdSpi]: Requesting query order info: " << endl << "\t[brokerId]: " << req.BrokerID << endl << "\t[investerId]: " << req.InvestorID << endl;
	
	m_pUserTDApi_trade->ReqQryOrder(&req, GetNextRequestId());
}

void TdSpi::ReqQryTrade()
{
	cerr << "[TdSpi]: Requesting trade query" << endl;

	CThostFtdcQryTradeField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.InvestorID, m_UserId.c_str());

	cerr << "[TdSpi]: Requesting trade query info: " << endl << "\t[brokerId]: " << req.BrokerID << endl << "\t[investerId]: " << req.InvestorID << endl;

	m_pUserTDApi_trade->ReqQryTrade(&req, GetNextRequestId());
}

void TdSpi::ReqQryInvestorPositionDetail()
{
	cerr << "[TdSpi]: Requesting investor position detail" << endl;

	CThostFtdcQryInvestorPositionDetailField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.InvestorID, m_UserId.c_str());

	cerr << "[TdSpi]: Requesting investor position detail info: " << endl << "\t[brokerId]: " << req.BrokerID << endl << "\t[investerId]: " << req.InvestorID << endl;

	m_pUserTDApi_trade->ReqQryInvestorPositionDetail(&req, GetNextRequestId());
}

void TdSpi::ReqQryTradingAccount()
{
	cerr << "[TdSpi]: Requesting query trading account" << endl;

	CThostFtdcQryTradingAccountField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.InvestorID, m_UserId.c_str());

	cerr << "[TdSpi]: Requesting query trading account: " << endl << "\t[brokerId]: " << req.BrokerID << endl << "\t[investerId]: " << req.InvestorID << endl;

	int iResult = m_pUserTDApi_trade->ReqQryTradingAccount(&req, GetNextRequestId());
	cerr << "[TdSpi]: Requesting query trading account done " << ((iResult == 0) ? "successfully" : "with failure") << endl;
}

void TdSpi::ReqQryInvestorPositionAll()
{
	cerr << "[TdSpi]: Requesting query investor position all" << endl;
	ReqQryInvestorPosition("");
}

void TdSpi::ReqQryInvestorPosition(string instrument)
{
	CThostFtdcQryInvestorPositionField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.InvestorID, m_UserId.c_str());
	strcpy_s(req.InstrumentID, instrument.c_str());

	cerr << "[TdSpi]: Requesting investor position info: " << endl << "\t[brokerId]: " << req.BrokerID << endl << "\t[investerId]: " << req.InvestorID << endl 
		<< "\t[instrumentId]: " << req.InstrumentID << endl;

	int iResult = m_pUserTDApi_trade->ReqQryInvestorPosition(&req, GetNextRequestId());
	cerr << "[TdSpi]: Requesting query investor position done " << ((iResult == 0) ? "successfully" : "with failure") << endl;
}

void TdSpi::ReqQryInstrumetAll()
{
	cerr << "[TdSpi]: Requesting query instrument all" << endl;
	ReqQryInstrumet("");
}

void TdSpi::ReqQryInstrumet(string instrument)
{
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.InstrumentID, instrument.c_str());

	cerr << "[TdSpi]: Requesting query instrument: " << req.InstrumentID << endl;

	int iResult = m_pUserTDApi_trade->ReqQryInstrument(&req, GetNextRequestId()); //req结构体为0，查询所有合约
	cerr << "[TdSpi]: Requesting query instrument done " << ((iResult == 0) ? "successfully" : "with failure") << endl;
}

void TdSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{
		cerr << "[TdSpi]: Failed to authenticate due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << "]" << endl;
		return;
	}

	cerr << "[TdSpi]: Authenticate successfully" << endl;

	ReqUserLogin();
}

bool TdSpi::IsErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
{
	return pRspInfo && pRspInfo->ErrorID != 0;;
}

void TdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{
		cerr << "[TdSpi]: Failed to login user due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << "]" << endl;
		return;
	}

	if (!pRspUserLogin)
	{
		cerr << "[TdSpi]: Null pRspUserLogin callback parameter" << endl;
		return;
	}

	cerr << "[TdSpi]: User login successfully" << endl;

	m_nFrontID = pRspUserLogin->FrontID; 
	m_nSessionID = pRspUserLogin->SessionID;

	int nextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
	sprintf_s(orderRef, sizeof(orderRef), "%012d",++nextOrderRef);

	cerr << "-------------Printing Login Info---------" << endl;

	cerr << "\t[FrontId]: " << pRspUserLogin->FrontID << endl 
		 << "\t[SessionId]: " << pRspUserLogin->SessionID << endl 
		 << "\t[MaxOrderRef]: " << pRspUserLogin->MaxOrderRef << endl
		 << "\t[SHFE Time]: " << pRspUserLogin->SHFETime << endl 
		 << "\t[DCE Time]: " << pRspUserLogin->DCETime << endl 
		 << "\t[CZCE Time]: "<< pRspUserLogin->CZCETime << endl 
		 << "\t[FFEX Time]: " << pRspUserLogin->FFEXTime << endl 
		 << "\t[INE Time]: " << pRspUserLogin->INETime << endl 
		 << "\t[Trade Day]: " << m_pUserTDApi_trade->GetTradingDay() << endl;
	
	strcpy_s(m_cTradingDay, m_pUserTDApi_trade->GetTradingDay()); // 设置交易日期

	cout<<"--------------------------------------------" << endl << endl;

	ReqSettlementInfoConfirm();
}

void TdSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void TdSpi::ReqSettlementInfoConfirm()
{
	cerr << "[TdSpi]: Requesting settlement info confirm" << endl;

	CThostFtdcSettlementInfoConfirmField req; 
	memset(&req, 0, sizeof(req)); 
	strcpy_s(req.BrokerID, m_BrokerId.c_str());
	strcpy_s(req.InvestorID, m_UserId.c_str()); 

	cerr << "[TdSpi]: Requesting settlement info confirm: " << endl << "[brokerId]: " << req.BrokerID << endl << "[investerId]: " << req.InvestorID << endl;

	int iResult = m_pUserTDApi_trade->ReqSettlementInfoConfirm(&req, GetNextRequestId());
	
	cerr << "[TdSpi]: Requesting settlement info confirm done " << ((iResult == 0) ? "successfully" : "with failure") << endl;
}

void TdSpi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void TdSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void TdSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[TdSpi]: Received response on query settlement info confirm" << endl;

	if (IsErrorRspInfo(pRspInfo) || !pSettlementInfoConfirm)
	{
		if (!pSettlementInfoConfirm)
		{
			cerr << "[TdSpi]: Null pSettlementInfoConfirm callback parameter" << endl;
		}
		else
		{
			cerr << "[TdSpi]: Failed to settle info confirmation due to" << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << "]" << endl;
		}
		
		if (bIsLast)
		{
			cerr << "[TdSpi]: Even though receiving unexpected result, still try to query order" << endl;
			waitAndPerformNextQuery(bind(&TdSpi::ReqQryOrder, this));
		}

		return;
	}

	if (!pSettlementInfoConfirm)
	{
		cerr << "[TdSpi]: Null pSettlementInfoConfirm returned" << endl;
		return;
	}

	cerr << "[TdSpi]: Settlement info confirmed successfully [investorId: " << pSettlementInfoConfirm->InvestorID << ", confirm date: " << pSettlementInfoConfirm->ConfirmDate 
		 << ", confirm time: " << pSettlementInfoConfirm->ConfirmTime << "]" << endl << endl;
	cerr << "[TdSpi]: First time query order" << endl;
	
	//线程休眠3秒，让ctp柜台有充足的响应时间，然后再进行查询操作
	waitAndPerformNextQuery(bind(&TdSpi::ReqQryOrder, this));
}

void TdSpi::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void TdSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
}

void TdSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	//cerr<<"请求查询持仓响应：OnRspQryInvestorPosition"<<",pInvestorPosition"<<pInvestorPosition<<endl;
	cerr << "Received response from query investor position" << endl;
	if (IsErrorRspInfo(pRspInfo) || !pInvestorPosition)
	{
		if (!pInvestorPosition)
		{
			cerr << "[TdSpi]: Null pInvestorPosition returned" << endl;
		}
		else
		{
			cerr << "[TdSpi]: Failed to query investor position due to" << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << "]" << endl;
		}

		if (bIsLast)
		{
			cerr << "[TdSpi]: Even though receiving unexpected result, still try to query all instruments" << endl;
			m_QryPosition_Once = false;
			waitAndPerformNextQuery(bind(&TdSpi::ReqQryInstrumetAll, this));
		}

		return;
	}

	if (!m_QryPosition_Once)
	{
		if (bIsLast)
		{
			cerr << "[TdSpi]: Not the first time query but still try to query instrument" << endl;
			waitAndPerformNextQuery(bind(&TdSpi::ReqQryInstrumetAll, this));
		}

		return;
	}

	//账户下所有合约
	cerr << "\t[Current Instrument ID]: " << pInvestorPosition->InstrumentID << endl
		 << "\t[Position Direction]: " << pInvestorPosition->PosiDirection << endl //2多3空
		//<< "映射后的方向" << MapDirection(pInvestorPosition->PosiDirection-2,false) << endl
		 << "\t[Position]: " << pInvestorPosition->Position << endl
		 << "\t[Today Position]: " << pInvestorPosition->TodayPosition << endl
		 << "\t[Real Yesterday Position]: " << pInvestorPosition->YdPosition << endl
		 << "\t[Margin]: " << pInvestorPosition->UseMargin << endl
		 << "\t[Position Cost]: " << pInvestorPosition->PositionCost << endl
		 << "\t[Open Volume]: " << pInvestorPosition->OpenVolume << endl
		 << "\t[Close Volume]: " << pInvestorPosition->CloseVolume << endl
		 << "\t[Trading Day]: " << pInvestorPosition->TradingDay << endl
		 << "\t[Close Profit Before Today]: " << pInvestorPosition->CloseProfitByDate << endl
		 << "\t[Position Profit]: " << pInvestorPosition->PositionProfit << endl
		 //<< "逐日盯市平仓盈亏（按昨结）" << pInvestorPosition->CloseProfitByDate << endl//快期中显示的是这个值
		 << "\t[Close Profit By Trade]: " << pInvestorPosition->CloseProfitByTrade << endl //在交易中比较有意义
		 << endl;

	//构造合约对应持仓明细信息的结构体map 
	//bool find_trade_message_map = false;
	//for (auto &it : m_position_field_map)
	//{
	//	if (!strcmp((it.first).c_str(), pInvestorPosition->InstrumentID)) //合约已存在
	//	{
	//		find_trade_message_map = true;
	//		break;
	//	}
	//}

	if (m_position_field_map.find(pInvestorPosition->InstrumentID) == m_position_field_map.end()) //合约不存在
	{
		cerr << "-----------------------Instrument ID not in the map yet" << endl;
		position_field* p_trade_message = new position_field();
		p_trade_message->instId = pInvestorPosition->InstrumentID;
		m_position_field_map[pInvestorPosition->InstrumentID] = p_trade_message;
	}

	position_field* p_tdm = m_position_field_map[pInvestorPosition->InstrumentID];
	if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)//多单
	{
		//昨仓和今仓一次返回
		//获取该合约的持仓明细信息结构体second; m_map[键]
		p_tdm->LongPosition = pInvestorPosition->Position;
		p_tdm->TodayLongPosition = pInvestorPosition->TodayPosition;
		p_tdm->YdLongPosition = p_tdm->LongPosition - p_tdm->TodayLongPosition;
		p_tdm->LongCloseProfit = pInvestorPosition->CloseProfit;
		p_tdm->LongPositionProfit = pInvestorPosition->PositionProfit;
	}
	else if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short)//空单
	{
		//昨仓和今仓一次返回
		p_tdm->ShortPosition = pInvestorPosition->Position;
		p_tdm->TodayShortPosition = pInvestorPosition->TodayPosition;
		p_tdm->YdShortPosition = p_tdm->ShortPosition - p_tdm->TodayShortPosition;
		p_tdm->ShortCloseProfit = pInvestorPosition->CloseProfit;
		p_tdm->ShortPositionProfit = pInvestorPosition->PositionProfit;
	}

	if (!bIsLast)
	{
		return;
	}

	m_QryPosition_Once = false;
	cerr << "--------------Start Printing Position------------------" << endl;
	for (auto& it : m_position_field_map)
	{
		cerr << "\t[Instrument ID]: " << it.second->instId << endl
			 << "\t[Long Position]: " << it.second->LongPosition << endl
			 << "\t[Short Position]: " << it.second->ShortPosition << endl
			 << "\t[Today Long Position]: " << it.second->TodayLongPosition << endl
			 << "\t[Real Yesterday Long Position]: " << it.second->YdLongPosition << endl
			 << "\t[Today Short Position]: " << it.second->TodayShortPosition << endl
			 << "\t[Real Yesterday Short Position]: " << it.second->YdShortPosition << endl
			 << "\t[Long Position Profit]: " << it.second->LongPositionProfit << endl
			 << "\t[Long Close Profit]: " << it.second->LongCloseProfit << endl
			 << "\t[Short Position Profit]: " << it.second->ShortPositionProfit << endl
			 << "\t[Short Close Profit]: " << it.second->ShortCloseProfit << endl << endl; //账户平仓盈亏

		m_CloseProfit = m_CloseProfit + it.second->LongCloseProfit + it.second->ShortCloseProfit; //账户浮动盈亏
		m_OpenProfit = m_OpenProfit + it.second->LongPositionProfit + it.second->ShortPositionProfit;
	}

	cerr << "--------------Finished Printing Position------------------" << endl;

	cerr << "[TdSpi]: Total Open Profit: " << m_OpenProfit << endl;
	cerr << "[TdSpi]: Total Close Profit: " << m_CloseProfit << endl;

	cerr << "[TdSpi]: Successfully queried investor positions" << endl;
	cerr << "[TdSpi]: Query all instruments" << endl;

	waitAndPerformNextQuery(bind(& TdSpi::ReqQryInstrumetAll, this));
}

void TdSpi::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo) || !pInstrument)
	{
		m_QryInstrument_Once = false;
		cerr << "[TdSpi]: Failed to query instrument" << endl;
		return;
	}

	if (!m_QryInstrument_Once)
	{
		cerr << "[TdSpi]: We only support query once and it is not the first time query" << endl;
		return;
	}

	//账户下所有合约
	m_Instrument_All += string(pInstrument->InstrumentID) + ",";//保存所有合约信息到map
	CThostFtdcInstrumentField* pInstField = new CThostFtdcInstrumentField();
	memcpy(pInstField, pInstrument, sizeof(CThostFtdcInstrumentField));
	m_inst_field_map[pInstrument->InstrumentID] = pInstField;

	//策略交易的合约
	/*if (!strcmp(m_InstId.c_str(), pInstrument->InstrumentID))
	{
		cerr << "响应|合约：" << pInstrument->InstrumentID << endl
			<< "合约名称：" << pInstrument->InstrumentName << endl
			<< "合约在交易所代码：" << pInstrument->ExchangeInstID << endl
			<< "产品代码：" << pInstrument->ProductID << endl
			<< "产品类型：" << pInstrument->ProductClass << endl
			<< "多头保证金率：" << pInstrument->LongMarginRatio << endl
			<< "空头保证金率：" << pInstrument->ShortMarginRatio << endl
			<< "合约数量乘数：" << pInstrument->VolumeMultiple << endl
			<< "最小变动价位：" << pInstrument->PriceTick << endl
			<< "交易所代码：" << pInstrument->ExchangeID << endl
			<< "交割年份：" << pInstrument->DeliveryYear << endl
			<< "交割月：" << pInstrument->DeliveryMonth << endl
			<< "创建日：" << pInstrument->CreateDate << endl
			<< "到期日：" << pInstrument->ExpireDate << endl
			<< "上市日：" << pInstrument->OpenDate << endl
			<< "开始交割日：" << pInstrument->StartDelivDate << endl
			<< "结束交割日：" << pInstrument->EndDelivDate << endl
			<< "合约生命周期状态：" << pInstrument->InstLifePhase << endl
			<< "当前是否交易：" << pInstrument->IsTrading << endl;
	}*/

	if (!bIsLast)
	{
		return;
	}

	m_QryInstrument_Once = false;
	m_Instrument_All = m_Instrument_All.substr(0, m_Instrument_All.length() - 1);
	//cerr << "[TdSpi]: instrument(s) is(are) " << m_Instrument_All << endl;
	cerr << "[TdSpi]: Contract count: " << m_inst_field_map.size() << endl;

	//将合约信息结构体的map复制到策略类
	g_strategy->set_instPostion_map_stgy(m_inst_field_map);

	//ShowInstMessage();
	//保存全市场合约，在TD进行，需要订阅全市场合约行情时再运行
	//m_pUserApi_md->setInstIDListAll(m_Instrument_All);
	cerr << "[TdSpi]: Finished initializing TD, start initialize MD" << endl;
	if (!m_pUserMDSpi_trade->hasRegisteredForCallback(observerID))
	{
		observerID = m_pUserMDSpi_trade->addObserverCallback(bind(&TdSpi::updatePositionMDPrice, this, placeholders::_1));
	}

	m_pUserMDApi_trade->Init();
}

void TdSpi::OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[TdSpi]: Received response on query order" << endl;

	if (IsErrorRspInfo(pRspInfo) || !pOrder)
	{
		// 查询报单错误或之前没有成交
		if (!pOrder)
		{
			cerr << "[TdSpi]: No former order" << endl;
		}
		else
		{
			cerr << "[TdSpi]: Failed to query order due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << ", pOrder: " << pOrder << "]" << endl;
		}

		m_QryOrder_Once = false;
		waitAndPerformNextQuery(bind(&TdSpi::ReqQryTrade, this));
		return;
	}

	cout << "[TdSpi]: Query order successfully [FrontID: " << pOrder->FrontID << ", SessionID: " << pOrder->SessionID << ", OrderRef: " << pOrder->OrderRef << "]" << endl;

	if (!m_QryOrder_Once)
	{
		return;
	}

	addOrderToOrderMap(pOrder);
	if (!bIsLast)
	{
		return;
	}

	m_QryOrder_Once = false;
	unordered_map<int, CThostFtdcOrderField*> _orderMap = GetOrderMap();
	cerr << "[TdSpi]: Order count: " << _orderMap.size() << endl;
	cerr << "---------------Printing Order-------------" << endl;
	for (auto &it : _orderMap)
	{
		cerr << "\t[BrokerID]: " << it.second->BrokerID << endl
			<< "\t[InvestorID]: " << it.second->InvestorID << endl
			<< "\t[UserID]: " << it.second->UserID << endl
			<< "\t[InstrumentID]: " << it.second->InstrumentID << endl
			<< "\t[Direction]: " << it.second->Direction << endl
			<< "\t[LimitPrice]: " << it.second->LimitPrice << endl
			<< "\t[VolumeTotal]: " << it.second->VolumeTotal << endl
			<< "\t[OrderRef]: " << it.second->OrderRef << endl
			<< "\t[ClientID]: " << it.second->ClientID << endl
			<< "\t[OrderStatus]: " << it.second->OrderStatus << endl
			<< "\t[ActiveTime]: " << it.second->ActiveTime << endl
			<< "\t[OrderStatus]: " << it.second->OrderStatus << endl
			<< "\t[TradingDay]: " << it.second->TradingDay << endl
			<< "\t[InsertDate]: " << it.second->InsertDate << endl
			<< endl;
	}

	cerr << "-------------Printing Finished------------" << endl;
	cerr << "[TdSpi]: All orders queried successfully, query trades now" << endl;

	waitAndPerformNextQuery(bind(&TdSpi::ReqQryTrade, this));
}

void TdSpi::OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[TdSpi]: Received response on query trade" << endl;
	if (IsErrorRspInfo(pRspInfo) || !pTrade)
	{
		// 查询成交错误或之前没有成交
		if (!pTrade)
		{
			cerr << "[TdSpi]: No former trade" << endl;
		}
		else
		{
			cerr << "[TdSpi]: Failed to query trade due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << ", pOrder: " << pTrade << "]" << endl;
		}

		m_QryTrade_Once = false;
		waitAndPerformNextQuery(bind(&TdSpi::ReqQryInvestorPositionDetail, this));
		return;
	}

	cout << "[TdSpi]: Query trade successfully" << endl;

	if (!m_QryTrade_Once)
	{
		return;
	}

	addTradeToList(pTrade, tradeList);
	if (!bIsLast)
	{
		return;
	}

	m_QryTrade_Once = false;
	cerr << "[TdSpi]: Trade count: " << tradeList.size() << endl;
	cerr << "---------------Printing Trade---------------" << endl;
	for (CThostFtdcTradeField* pTrade : tradeList)
	{
		cerr << "\t[BrokerID]: " << pTrade->BrokerID << endl
			 << "\t[InvestorID]: " << pTrade->InvestorID << endl
			 << "\t[UserID]: " << pTrade->UserID << endl
			 << "\t[TradeID]: " << pTrade->TradeID << endl
			 << "\t[InstrumentID]: " << pTrade->InstrumentID << endl
			 << "\t[Direction]: " << pTrade->Direction << endl
			 << "\t[OffsetFlag]: " << pTrade->OffsetFlag << endl
			 << "\t[HedgeFlag]: " << pTrade->HedgeFlag << endl
			 << "\t[Price]: " << pTrade->Price << endl
			 << "\t[Volume]: " << pTrade->Volume << endl
			 << "\t[OrderRef]: " << pTrade->OrderRef << endl
			 << "\t[OrderLocalID]: " << pTrade->OrderLocalID << endl
			 << "\t[Trade Time]: " << pTrade->TradeTime << endl
			 << "\t[BusinessUnit]: " << pTrade->BusinessUnit << endl
			 << "\t[SequenceNo]: " << pTrade->SequenceNo << endl
			 << "\t[BrokerOrderSeq]: " << pTrade->BrokerOrderSeq << endl
			 << "\t[Trade Day]: " << pTrade->TradingDay << endl
			 << endl;
	}

	cerr << "---------------Printing Finished---------------" << endl;
	cerr << "[TdSpi]: All trades queried successfully, query investor position now" << endl;
	
	//线程休眠3秒，让ctp柜台有充足的响应时间，然后再进行查询操作
	waitAndPerformNextQuery(bind(&TdSpi::ReqQryInvestorPositionDetail, this));
}

void TdSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[TdSpi]: Received response on query investor position detail" << endl;

	if (IsErrorRspInfo(pRspInfo) || !pField)
	{
		// 查询成交错误或之前没有成交
		if (!pField)
		{
			cerr << "[TdSpi]: No former position detail" << endl;
		}
		else
		{
			cerr << "[TdSpi]: Failed to query trade due to " << "[errorId: " << pRspInfo->ErrorID << ", errorMsg: " << pRspInfo->ErrorMsg << ", pField: " << pField << "]" << endl;
		}

		m_QryTrade_Once = false;
		waitAndPerformNextQuery(bind(&TdSpi::ReqQryTradingAccount, this));
		return;
	}

	//所有合约
	if (m_QryDetail_Once)
	{
		//对于所有合约，只保存未平仓的，不保存已经平仓的
		//将程序启动前的持仓记录保存到未平仓容器positionDetailList_NotClosed_Long和positionDetailList_NotClosed_Short
		//使用结构体CThostFtdcTradeField，因为它有时间字段，而CThostFtdcInvestorPositionDetailField没有时间字段
		CThostFtdcTradeField* trade = new CThostFtdcTradeField();
		strcpy_s(trade->InvestorID, pField->InvestorID); ///投资者代码
		strcpy_s(trade->InstrumentID, pField->InstrumentID); ///合约代码
		strcpy_s(trade->ExchangeID, pField->ExchangeID); ///交易所代码
		trade->Direction = pField->Direction; //买卖方向
		trade->Price = pField->OpenPrice; //价格
		trade->Volume = pField->Volume; //数量
		strcpy_s(trade->TradeDate, pField->OpenDate); //成交日期
		strcpy_s(trade->TradeID, pField->TradeID); //成交编号
		if (pField->Volume > 0)//筛选未平仓合约
		{
			if (trade->Direction == THOST_FTDC_D_Buy) //买入方向
			{
				positionDetailList_NotClosed_Long.push_front(trade);
			}
			else if (trade->Direction == THOST_FTDC_D_Sell) //卖出方向
			{
				positionDetailList_NotClosed_Short.push_front(trade);
			}
		}

		//收集持仓合约的代码
		bool find_instId = false;
		vector<char *> subscribe_inst_vec = m_pUserMDSpi_trade->getSubscribedInstrVec();
		for (char *inst : subscribe_inst_vec)
		{
			if (!strcmp(inst, trade->InstrumentID))//合约已存在，已订阅
			{
				find_instId = true;
				break;
			}
		}

		if (!find_instId) //合约未订阅过
		{
			cerr << "[TdSpi]: New instrument, subscribe market data for this open position" << endl;
			m_pUserMDSpi_trade->addInstrToSubscrbedInstrVec(trade->InstrumentID);
			//subscribe_inst_vec.push_back(trade->InstrumentID);
		}
	}

	if (!bIsLast)
	{
		return;
	}

	//输出所有合约的持仓明细，要在这边进行下一步的查询ReqQryTradingAccount()
	m_QryDetail_Once = false;
	string inst_holding;
	vector<char *> subscribe_inst_vec = m_pUserMDSpi_trade->getSubscribedInstrVec();
	for (char *inst : subscribe_inst_vec)
	{
		inst_holding += string(inst) + ",";
	}

	inst_holding = inst_holding.substr(0, inst_holding.length() - 1);//去掉最后的逗号，从位置0开始，选取length-1个字符

	cerr << "[TdSpi]: Open position instruments before trade starts: " << inst_holding << endl;
	if (inst_holding.length() > 0)
	{
		m_pUserMDSpi_trade->setInstIDList_Position_MD(inst_holding);//设置程序启动前的留仓，即需要订阅行情的合约
	}

	cerr << "[TdSpi]: Open long position count: " << positionDetailList_NotClosed_Long.size() << ", Open short position count: " << positionDetailList_NotClosed_Short.size() << endl;
	cerr << "-----------------Start Printing Open Long Position----------------" << endl;
	for (CThostFtdcTradeField *trade : positionDetailList_NotClosed_Long)
	{
		cerr << "\t[BrokerID]: " << trade->BrokerID << endl
			 << "\t[InvestorID]: " << trade->InvestorID << endl
			 << "\t[InstrumentID]: " << trade->InstrumentID << endl
			 << "\t[Direction]: " << trade->Direction << endl
			 << "\t[OpenPrice]: " << trade->Price << endl
			 << "\t[Volume]: " << trade->Volume << endl
			 << "\t[TradeDate]: " << trade->TradeDate << endl
			 << "\t[TradeID]: " << trade->TradeID << endl 
			 << endl;
	}

	cerr << "-----------------Start Printing Open Short Position----------------" << endl;
	for (CThostFtdcTradeField *trade : positionDetailList_NotClosed_Short)
	{
		cerr << "\t[BrokerID] : " << trade->BrokerID << endl
			 << "\t[InvestorID]: " << trade->InvestorID << endl
			 << "\t[InstrumentID]: " << trade->InstrumentID << endl
			 << "\t[Direction]: " << trade->Direction << endl
			 << "\t[OpenPrice]: " << trade->Price << endl
			 << "\t[Volume]: " << trade->Volume << endl
			 << "\t[TradeDate]: " << trade->TradeDate << endl
			 << "\t[TradeID]: " << trade->TradeID << endl
			 << endl;
	}

	cerr << "---------------Finished Printing---------------" << endl;
	cerr << "[TDSpi]: Successfully queried position details, start querying trading account" << endl;

	//线程休眠3秒，让ctp柜台有充足的响应时间，然后再进行查询操作
	waitAndPerformNextQuery(bind(&TdSpi::ReqQryTradingAccount, this));
}

void TdSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "[TdSpi]: Received response on query trading account" << endl;

	if (IsErrorRspInfo(pRspInfo) || !pTradingAccount || !m_QryTradingAccount_Once)
	{
		cerr << "[TdSpi]: Either unexpected result, or null pTradingAccount, not the first time query. Still proceed to query trading account" << endl;
		m_QryTradingAccount_Once = false;
		waitAndPerformNextQuery(bind(&TdSpi::ReqQryInvestorPositionAll, this));
		return;
	}

	cerr << "[TdSpi]: Successfully queried trading account" << endl;

	if (m_QryTradingAccount_Once)
	{
		cerr << "-------------Printing Trading Account----------------" << endl;

		cerr << "\t[AccountID]: " << pTradingAccount->AccountID << endl
			 << "\t[PreBalance]: " << pTradingAccount->PreBalance << endl
			 << "\t[Balance]: " << pTradingAccount->Balance << endl
			 << "\t[Available]: " << pTradingAccount->Available << endl
			 << "\t[WithdrawQuota]: " << pTradingAccount->WithdrawQuota << endl
			 << "\t[CurrMargin]: " << pTradingAccount->CurrMargin << endl
			 << "\t[CloseProfit]: " << pTradingAccount->CloseProfit << endl
			 << "\t[PositionProfit]: " << pTradingAccount->PositionProfit << endl
			 << "\t[Commission]: " << pTradingAccount->Commission << endl
			 << "\t[FrozenCash]: " << pTradingAccount->FrozenCash << endl;

		cerr << "-------------Finished Printing----------------" << endl;
	}

	m_QryTradingAccount_Once = false;

	cerr << "[TdSpi]: Query investor position all next" << endl;

	//线程休眠3秒，让ctp柜台有充足的响应时间，然后再进行查询操作
	waitAndPerformNextQuery(bind(&TdSpi::ReqQryInvestorPositionAll, this));
}

void TdSpi::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	if (!pOrder)
	{
		cerr << "[TdSpi]: Received null pOrder" << endl;
		return;
	}

	printf("[TdSpi]: Received order returned, ins: %s, vol: %d, price: %f, orderref: %s, requestid: %d, tradedvol: %d, ExchID: %s, OrderSysID: %s, statusmsg: %s\n",
			pOrder->InstrumentID, pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->OrderRef, pOrder->RequestID, pOrder->VolumeTraded,
			pOrder->ExchangeID, pOrder->OrderSysID, pOrder->StatusMsg);

	char* pszStatus = new char[20];
	switch (pOrder->OrderStatus)
	{
	case THOST_FTDC_OST_AllTraded:
		strcpy(pszStatus, "All Traded");
		break;
	case THOST_FTDC_OST_PartTradedQueueing:
		strcpy(pszStatus, "Partially Traded");
		break;
	case THOST_FTDC_OST_NoTradeQueueing:
		strcpy(pszStatus, "No Trade");
		break;
	case THOST_FTDC_OST_Canceled:
		strcpy(pszStatus, "Order Cancelled");
		break;
	case THOST_FTDC_OST_Unknown:
		strcpy(pszStatus, "Unknown");
		break;
	case THOST_FTDC_OST_NotTouched:
		strcpy(pszStatus, "Not Touched");
		break;
	case THOST_FTDC_OST_Touched:
		strcpy(pszStatus, "Touched");
		break;
	default:
		strcpy(pszStatus, "Wrong Status");
		break;
	}

	cerr << "[TdSpi]: The order status is: " << pszStatus << endl;

	// 处理全部报单
	UpdateOrder(pOrder, orderMap);

	// 打印所有程序报单
	ShowOrders(GetOrderMap());

	// 处理本程序报单
	// 判断是否是本程序发出的报单
	if (pOrder->FrontID != m_nFrontID || pOrder->SessionID != m_nSessionID)
	{
		cerr << "[MdSpi]: The return order is not from this process" << endl;
		CThostFtdcOrderField* order = GetOrder(pOrder->BrokerOrderSeq);
		if (!order)
		{
			cerr << "[MdSpi]: This return order is not part of our trades, skip it..." << endl;
			return;
		}
	}

	// 首先保存并更新这个报单
	UpdateOrder(pOrder, m_Orders);

	// 打印此程序的报单
	ShowOrders(GetmOrders());

	g_strategy->OnRtnOrder(pOrder);

	delete[] pszStatus;
}

void TdSpi::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	if (!pTrade)
	{
		cerr << "[TdSpi]: Received null pTrade" << endl;
		return;
	}

	printf("[TdSpi]: Received trade returned, ins: %s, tradevol: %d, tradeprice: %f, ExchID: %s, OrderSysID: %s\n",
		pTrade->InstrumentID, pTrade->Volume, pTrade->Price, pTrade->ExchangeID, pTrade->OrderSysID);

	// 更新合约信息
	updatePositionFieldMap(pTrade->InstrumentID);

	// 保存成交到tradeList，账户所有的成交列表
	CThostFtdcTradeField* trade = GetTrade(pTrade, GetTradeList());
	if (trade)
	{
		// 断线重连可能会发生这种情况
		cerr << "[TdSpi]: The trade already existed in our tradeMap" << endl;
		return;
	}

	addTradeToList(pTrade, tradeList);
	ShowTradeList(GetTradeList());

	// 更新持仓明细
	updatePositionDetail(pTrade);
	ShowPositionDetail();

	// 检查合约是否订阅
	if (!m_pUserMDSpi_trade->hasSubscribedInstr(pTrade->InstrumentID))
	{
		m_pUserMDSpi_trade->addInstrToSubscrbedInstrVec(pTrade->InstrumentID);
		m_pUserMDSpi_trade->SubscribeMarketData(pTrade->InstrumentID);
	}
	
	// 更新持仓信息
	updatePosition("");

	// 如果有更新，打印持仓信息
	if (isMDPriceUpdated())
	{
		ShowPosition();
	}
	
	// 更新本程序的成交信息
	// 判断是否为断续重连
	trade = GetTrade(pTrade, GetmTrades());
	if (trade)
	{
		// 断线重连可能会发生这种情况
		cerr << "[TdSpi]: The trade already existed in our m_Trades" << endl;
		return;
	}

	//判断是否本程序发出的报单；
	CThostFtdcOrderField *pOrder = GetOrder(pTrade->BrokerOrderSeq);
	if (!pOrder)
	{
		cerr << "[TdSpi]: Do not find the corresponding order from the trade" << endl;
		return;
	}
	
	addTradeToList(pTrade, m_Trades);
	ShowTradeList(GetmTrades());

	g_strategy->OnRtnTrade(pTrade);
}

void TdSpi::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	printf("[TdSpi]: Failed to insert order (OnRspOrderInsert), ins: %s, vol: %d, price: %f, orderref: %s, requestid: %d, ErrorID: %d, errmsg: %s\n", 
			pInputOrder->InstrumentID, pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->OrderRef, 
			pInputOrder->RequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);

	g_strategy->OnRspOrderInsert(pInputOrder);
}

void TdSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo)
{
	printf("[TdSpi]: Failed to insert order (OnErrRtnOrderInsert), ins: %s, vol: %d, price: %f, orderref: %s, requestid: %d, ErrorID: %d, errmsg: %s\n", 
			pInputOrder->InstrumentID, pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->OrderRef, pInputOrder->RequestID,
			pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

void TdSpi::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	printf("[TdSpi]: Failed to insert order due to improper action, ins: %s, vol: %d, price: %f, orderref: %s, orderActionref: %d, requestid: %d, ErrorID: %d, errmsg:%s\n", 
			pInputOrderAction->InstrumentID, pInputOrderAction->VolumeChange, pInputOrderAction->LimitPrice, pInputOrderAction->OrderRef, 
			pInputOrderAction->OrderActionRef, pInputOrderAction->RequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

void TdSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
{
	printf("[TdSpi]: Failed to insert order due to improper action, ins: %s, vol: %d, price: %f, orderref: %s, orderActionref: %d, requestid: %d, ErrorID: %d, errmsg:%s\n", 
			pOrderAction->InstrumentID, pOrderAction->VolumeChange, pOrderAction->LimitPrice, pOrderAction->OrderRef, pOrderAction->OrderActionRef, 
			pOrderAction->RequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

CThostFtdcOrderField* TdSpi::PlaceOrder(const char* pszCode, const char* pszExchangeID, const char nDirection, int nOpenClose, int nVolume, double fPrice)
{
	if (!pszCode || nDirection < THOST_FTDC_D_Buy || nDirection > THOST_FTDC_D_Sell)
	{
		cerr << "[TdSpi]: Invalid parameter passed-in" << endl;
		return nullptr;
	}
	CThostFtdcInputOrderField orderField; 
	memset(&orderField, 0, sizeof(CThostFtdcInputOrderField));
	
	//fill the broker and user fields;
	strcpy(orderField.BrokerID, m_BrokerId.c_str());
	strcpy(orderField.InvestorID, m_UserId.c_str());
	
	// 下单的期货合约;
	strcpy(orderField.InstrumentID, pszCode);
	CThostFtdcInstrumentField *instField = m_inst_field_map.find(pszCode)->second;
	/*if (instField)
	{
		constchar* ExID = instField->ExchangeID;
		strcpy(orderField.ExchangeID, ExID);
	}*/
	
	// 交易所代码
	strcpy(orderField.ExchangeID, pszExchangeID);
	orderField.Direction = nDirection;
	
	orderField.LimitPrice = fPrice; // 价格
	orderField.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation; // 投机还是套利
	orderField.VolumeTotalOriginal = nVolume; // 下单手数
	orderField.VolumeCondition = THOST_FTDC_VC_AV; // 任意数量

	//限价单；
	orderField.OrderPriceType = THOST_FTDC_OPT_LimitPrice; // 报单的价格类型
	orderField.TimeCondition = THOST_FTDC_TC_GFD; // 时间条件
	
	//市价单；
	//orderField.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
	//orderField.TimeCondition = THOST_FTDC_TC_IOC;

	strcpy(orderField.GTDDate, m_cTradingDay); // 下单日期
	orderField.ContingentCondition = THOST_FTDC_CC_Immediately; // 报单的触发条件
	orderField.ForceCloseReason = THOST_FTDC_FCC_NotForceClose; // 强平的原因
	
	switch (nOpenClose)
	{
		case 0:
			orderField.CombOffsetFlag[0] = THOST_FTDC_OF_Open; // 开仓
			break;
		case 1:
			orderField.CombOffsetFlag[0] = THOST_FTDC_OF_Close; // 平仓
			break;
		case 2:
			orderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday; // 平今仓
			break;
		case 3:
			orderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday; // 平昨仓
			break;
	}

	// 自增order ref
	strcpy(orderField.OrderRef, GetNextOrderRef());
	
	// 调用交易Api的Place Order
	int retCode = m_pUserTDApi_trade->ReqOrderInsert(&orderField, GetNextRequestId());
	if (retCode)
	{
		printf("[TdSpi]: Failed to insert order, instrument: %s, volume: %d, price: %f\n", pszCode, nVolume, fPrice);
	}

	// Create pOrder
	CThostFtdcOrderField* pOrder = new CThostFtdcOrderField();
	
	// 初始化清零
	memset(pOrder, 0, sizeof(CThostFtdcOrderField));
	strcpy(pOrder->BrokerID, orderField.BrokerID);
	strcpy(pOrder->InvestorID, orderField.InvestorID);
	strcpy(pOrder->InstrumentID, orderField.InstrumentID);
	strcpy(pOrder->ExchangeID, orderField.ExchangeID);
	strcpy(pOrder->GTDDate, orderField.GTDDate);
	strcpy(pOrder->OrderRef, orderField.OrderRef);

	pOrder->Direction = orderField.Direction;
	pOrder->LimitPrice = orderField.LimitPrice;
	pOrder->CombHedgeFlag[0] = orderField.CombHedgeFlag[0];
	pOrder->VolumeTotalOriginal = orderField.VolumeTotalOriginal;
	pOrder->OrderPriceType = orderField.OrderPriceType;
	pOrder->ContingentCondition = orderField.ContingentCondition;
	pOrder->TimeCondition = orderField.TimeCondition;
	pOrder->VolumeCondition = orderField.VolumeCondition;
	pOrder->ForceCloseReason = orderField.ForceCloseReason;
	pOrder->CombOffsetFlag[0] = orderField.CombOffsetFlag[0];
	pOrder->FrontID = m_nFrontID;
	pOrder->SessionID = m_nSessionID;

	string key = to_string(pOrder->FrontID) + to_string(pOrder->SessionID) + pOrder->OrderRef;
	m_mOrders[key] = pOrder;
	return pOrder;
}

void TdSpi::CancelOrder(CThostFtdcOrderField* pOrder)
{
	if (!pOrder)
	{
		cerr << "[TdSpi]: Cannot cancel the order due to nullptr pOrder" << endl;
		return;
	}

	CThostFtdcOrderField* order = GetOrder(pOrder->BrokerOrderSeq);
	if (order && (order->OrderStatus == '6' || order->OrderStatus == THOST_FTDC_OST_Canceled))
	{
		printf("[TdSpi]: The requested order %s was already cancelled or cancelling\n", order->InstrumentID);
		return;
	}

	CThostFtdcInputOrderActionField oa; 
	memset(&oa, 0, sizeof(CThostFtdcInputOrderActionField)); 
	// 经过测试，下面三个字段是确定我们的报单
	oa.ActionFlag = THOST_FTDC_AF_Delete; 
	oa.FrontID = pOrder->FrontID; 
	oa.SessionID = pOrder->SessionID; 
	strcpy(oa.OrderRef, pOrder->OrderRef); 


	if (pOrder->ExchangeID[0] != '\0') 
	{ 
		strcpy(oa.ExchangeID, pOrder->ExchangeID); 
	}
	
	if (pOrder->OrderSysID[0] != '\0') 
	{ 
		strcpy(oa.OrderSysID, pOrder->OrderSysID); 
	}
	
	strcpy(oa.BrokerID, pOrder->BrokerID); 
	strcpy(oa.UserID, pOrder->UserID); 
	strcpy(oa.InstrumentID, pOrder->InstrumentID); 
	strcpy(oa.InvestorID, pOrder->InvestorID);
	
	// oa.RequestID=pOrder->RequestID;
	oa.RequestID = GetNextRequestId();
	
	cerr << "[TdSpi]: Request to cancel the order " << pOrder->InstrumentID << "(" << pOrder->OrderRef << ")" << endl;
	int nRetCode = m_pUserTDApi_trade->ReqOrderAction(&oa,oa.RequestID);

	char *pszStatus = new char[20];
	switch (pOrder->OrderStatus)
	{
		case THOST_FTDC_OST_AllTraded:
			strcpy(pszStatus, "All Traded");
			break;
		case THOST_FTDC_OST_PartTradedQueueing:
			strcpy(pszStatus, "Partially Traded");
			break;
		case THOST_FTDC_OST_NoTradeQueueing:
			strcpy(pszStatus, "No Trade");
			break;
		case THOST_FTDC_OST_Canceled:
			strcpy(pszStatus, "Cancelled");
			break;
		case THOST_FTDC_OST_Unknown:
			strcpy(pszStatus, "Unknown");
			break;
		case THOST_FTDC_OST_NotTouched:
			strcpy(pszStatus, "Not Touched");
			break;
		case THOST_FTDC_OST_Touched:
			strcpy(pszStatus, "Touched");
			break;
		default:
			strcpy(pszStatus, "Wrong Status");
			break;
	}
	
	/*printf("撤单ing, ins: %s, vol: %d, price: %f, orderref: %s, requestid: %d, tradedvol: %d, ExchID: %s, OrderSysID: %s, status: %s, statusmsg: %s\n",
			 pOrder->InstrumentID, pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->OrderRef, pOrder->RequestID, pOrder->VolumeTraded, pOrder->ExchangeID,
			 pOrder->OrderSysID, pszStatus, pOrder->StatusMsg);*/
	
	if (nRetCode)
	{
		printf("[TdSpi]: Failed to cancel the order.\n");
	}
	else
	{
		printf("[TdSpi]: Successfully requested cancelling the order %c and mark the status to cancelling\n", pOrder->OrderStatus);
		if (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded && pOrder->OrderStatus != THOST_FTDC_OST_Canceled)
		{
			pOrder->OrderStatus = '6'; //‘6’表示撤单途中
			UpdateOrder(pOrder);
		}
	}
}

void TdSpi::ShowPosition()
{
	cerr << "-------------------Start printing position---------------------------" << endl;
	for (auto it : m_position_field_map)
	{
		if (!it.second->LongPosition && !it.second->ShortPosition)
		{
			continue;
		}

		//账户下所有合约
		cerr << "\t[Instrument ID]: " << it.second->instId << endl
			<< "\t[Long Position]: " << it.second->LongPosition << endl //2多3空
			<< "\t[Today Long Position]: " << it.second->TodayLongPosition << endl
			<< "\t[Yesterday Long Position]: " << it.second->YdLongPosition << endl
			<< "\t[Long Average Entry Price]: " << it.second->LongAvgEntryPrice << endl
			<< "\t[Long Average Holding Price]: " << it.second->LongAvgHoldingPrice << endl
			<< "\t[Long Open Position Profit]: " << it.second->LongPositionProfit << endl
			<< "\t[Long Close Profit]: " << it.second->LongCloseProfit << endl
			<< "\t[Short Position]: " << it.second->ShortPosition << endl
			<< "\t[Today Short Position]: " << it.second->TodayShortPosition << endl
			<< "\t[Yesterday Short Position]: " << it.second->YdShortPosition << endl
			<< "\t[Short Average Entry Price]: " << it.second->ShortAvgEntryPrice << endl
			<< "\t[Short Average Holding Price]: " << it.second->ShortAvgHoldingPrice << endl
			<< "\t[Short Position Profit]: " << it.second->ShortPositionProfit << endl
			<< "\t[Short Close Profit]: " << it.second->ShortCloseProfit << endl
			<< "\t[Last Price]: " << it.second->lastPrice << endl
			<< "\t[PreSettlement Price]: " << it.second->PreSettlementPrice << endl
			<< endl;
	}
}

void TdSpi::ClosePosition()
{
}

void TdSpi::SetAllowOpen(bool isOk)
{
}

CThostFtdcOrderField* TdSpi::GetOrder(int nBrokerOrderSeq)
{
	CThostFtdcOrderField *pOrder = nullptr; 
	lock_guard<std::mutex>m_lock(m_mutex); 
	unordered_map<int, CThostFtdcOrderField*>::iterator it = m_Orders.find(nBrokerOrderSeq);
	if (it != m_Orders.end()) 
	{ 
		pOrder = it->second; 
	}
	
	return pOrder;
}

bool TdSpi::UpdateOrder(CThostFtdcOrderField* pOrder)
{
	// 我们只更新接受的报单
	if (!pOrder->BrokerOrderSeq)
	{
		printf("[TdSpi]: Do not update order since it was from invalid broker order sequence, ins: %s, orderref: %s, requestid: %d\n",
				pOrder->InstrumentID, pOrder->OrderRef, pOrder->RequestID);
		return false;
	}

	std::lock_guard<std::mutex>m_lock(m_mutex); 
	unordered_map<int, CThostFtdcOrderField*>::iterator it = m_Orders.find(pOrder->BrokerOrderSeq);
	if (it != m_Orders.end()) 
	{
		CThostFtdcOrderField* pOld = it->second; //原报单已经关闭；
		char cOldStatus = pOld->OrderStatus;
		switch (cOldStatus)
		{
			case THOST_FTDC_OST_AllTraded:
			case THOST_FTDC_OST_Canceled:
			case THOST_FTDC_OST_Touched:
				return false;
			case'6': //canceling, 自己定义的，本程序已经发送了撤单请求，还在途中
				if (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded && pOrder->OrderStatus != THOST_FTDC_OST_Canceled)
				{
					return false;
				}
		}
			
		memcpy(pOld, pOrder, sizeof(CThostFtdcOrderField));
		printf("[TdSpi]: Order was updated, ins: %s, orderref: %s, requestid: %d, status: %c\n", pOrder->InstrumentID, pOrder->OrderRef, pOrder->RequestID, pOrder->OrderStatus);
	}
	else 
	{ 
		CThostFtdcOrderField *pNew = new CThostFtdcOrderField();
		memcpy(pNew, pOrder, sizeof(CThostFtdcOrderField));
		m_Orders[pNew->BrokerOrderSeq] = pNew;
		printf("[TdSpi]: Insert new order, ins: %s, orderref: %s, requestid: %d, status: %c\n", pOrder->InstrumentID, pOrder->OrderRef, pOrder->RequestID, pOrder->OrderStatus);
	}

	return true;
}

bool TdSpi::UpdateOrder(CThostFtdcOrderField* pOrder, std::unordered_map<int, CThostFtdcOrderField*> &orders)
{
	// 经纪公司序列号大于0代表此报单已经接受了
	if (!pOrder->BrokerOrderSeq)
	{
		printf("[TdSpi]: Do not update order since it was from invalid broker order sequence, ins: %s, orderref: %s, requestid: %d\n",
			pOrder->InstrumentID, pOrder->OrderRef, pOrder->RequestID);
		return false;
	}

	std::lock_guard<std::mutex>m_lock(m_mutex);
	unordered_map<int, CThostFtdcOrderField*>::iterator it = orders.find(pOrder->BrokerOrderSeq);
	if (it != orders.end())
	{
		CThostFtdcOrderField* pOld = it->second; //原报单已经关闭；
		char cOldStatus = pOld->OrderStatus;
		switch (cOldStatus)
		{
			case THOST_FTDC_OST_AllTraded:
			case THOST_FTDC_OST_Canceled:
			case THOST_FTDC_OST_Touched:
				return false;
			case'6': //canceling, 自己定义的，本程序已经发送了撤单请求，还在途中
				if (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded && pOrder->OrderStatus != THOST_FTDC_OST_Canceled)
				{
					return false;
				}
		}

		memcpy(pOld, pOrder, sizeof(CThostFtdcOrderField));
		printf("[TdSpi]: Order was updated, ins: %s, orderref: %s, requestid: %d, status: %c\n", pOrder->InstrumentID, pOrder->OrderRef, pOrder->RequestID, pOrder->OrderStatus);
	}
	else
	{
		CThostFtdcOrderField* pNew = new CThostFtdcOrderField();
		memcpy(pNew, pOrder, sizeof(CThostFtdcOrderField));
		orders[pNew->BrokerOrderSeq] = pNew;
		printf("[TdSpi]: Insert new order, ins: %s, orderref: %s, requestid: %d, status: %c\n", pOrder->InstrumentID, pOrder->OrderRef, pOrder->RequestID, pOrder->OrderStatus);
	}

	return true;
}

void TdSpi::waitAndPerformNextQuery(function<void()> queryFunc)
{
	//线程休眠3秒，让ctp柜台有充足的响应时间，然后再进行查询操作
	std::chrono::milliseconds sleepDuration(3 * 1000);
	std::this_thread::sleep_for(sleepDuration);
	queryFunc();
}

int TdSpi::GetNextRequestId()
{
	lock_guard<mutex> lock(m_mutex);
	return ++nRequestID;
}

char* TdSpi::GetNextOrderRef()
{
	lock_guard<mutex> lock(m_mutex);
	int ref = atoi(orderRef);
	sprintf_s(orderRef, sizeof(orderRef), "%012d", ++ref);
	return orderRef;
}

CThostFtdcOrderField* TdSpi::GetOrder(CThostFtdcOrderField* pOrder)
{
	if (!pOrder)
	{
		return nullptr;
	}

	lock_guard<mutex> lock(m_mutex);
	for (auto it : m_Orders)
	{
		if (pOrder->FrontID == it.second->FrontID && pOrder->SessionID == it.second->SessionID && !strcmp(pOrder->OrderRef, it.second->OrderRef))
		{
			return it.second;
		}
	}

	return nullptr;
}

void TdSpi::ShowInstMessage()
{
	cerr << "--------------Start Printing Instrument(s)-----------" << endl;
	for (auto &it : m_inst_field_map)
	{
		string instrumentID = it.first;
		CThostFtdcInstrumentField *pInstrument = it.second;
		cerr << "[InstrumentID: " << instrumentID << ", "
			<< "Contract Name: " << pInstrument->InstrumentName << ", "
			<< "Exchange Instrument ID: " << pInstrument->ExchangeInstID << ", "
			<< "ProductID: " << pInstrument->ProductID << ", " 
			<< "Product Class: " << pInstrument->ProductClass << ", " 
			<< "Long Margin Ratio: " << pInstrument->LongMarginRatio << ", " 
			<< "Short Margin Ratio: " << pInstrument->ShortMarginRatio << ", " 
			<< "Volume Multiple: " << pInstrument->VolumeMultiple << ", " 
			<< "Price Tick:" << pInstrument->PriceTick << ", " 
			<< "ExchangeID: " << pInstrument->ExchangeID << ", " 
			<< "Delivery Year: " << pInstrument->DeliveryYear << ", " 
			<< "Delivert Month: " << pInstrument->DeliveryMonth << ", " 
			<< "Create Date: " << pInstrument->CreateDate << ", " 
			<< "Expire Date: " << pInstrument->ExpireDate << ", " 
			<< "Open Date: " << pInstrument->OpenDate << ", " 
			<< "Start Delivery Date: " << pInstrument->StartDelivDate << ", " 
			<< "End Delivery Date: " << pInstrument->EndDelivDate << ", " 
			<< "Instrument Life Phase: " << pInstrument->InstLifePhase << ", " 
			<< "Is Trading: " << pInstrument->IsTrading << "]" << endl;
	}

	cerr << "-------------Finished Printing Instrument(s)----------" << endl;
}

void TdSpi::ShowOrders(unordered_map<int, CThostFtdcOrderField*> orders)
{
	cerr << "--------------Start Printing Orders-----------" << endl;
	for (auto& it : orders)
	{
		cerr << "\t[BrokerID]: " << it.second->BrokerID << endl
			<< "\t[InvestorID]: " << it.second->InvestorID << endl
			<< "\t[UserID]: " << it.second->UserID << endl
			<< "\t[InstrumentID]: " << it.second->InstrumentID << endl
			<< "\t[Direction]: " << it.second->Direction << endl
			<< "\t[LimitPrice]: " << it.second->LimitPrice << endl
			<< "\t[VolumeTotal]: " << it.second->VolumeTotal << endl
			<< "\t[OrderRef]: " << it.second->OrderRef << endl
			<< "\t[ClientID]: " << it.second->ClientID << endl
			<< "\t[OrderStatus]: " << it.second->OrderStatus << endl
			<< "\t[ActiveTime]: " << it.second->ActiveTime << endl
			<< "\t[OrderStatus]: " << it.second->OrderStatus << endl
			<< "\t[TradingDay]: " << it.second->TradingDay << endl
			<< "\t[InsertDate]: " << it.second->InsertDate << endl
			<< endl;
	}

	cerr << "-------------Finished Printing Orders----------" << endl;
}

void TdSpi::ShowTradeList(vector<CThostFtdcTradeField*> trades)
{
	cerr << "--------------Start Printing TradeList-----------" << endl;
	for (CThostFtdcTradeField *trade : trades)
	{
		cerr << "[Investor ID: " << trade->InvestorID << ", "
			<< "User ID: " << trade->UserID << ", "
			<< "Trade ID: " << trade->TradeID << ", "
			<< "Instrument ID: " << trade->InstrumentID << ", "
			<< "Direction: " << trade->Direction << ", "
			<< "Offset Flag: " << trade->OffsetFlag << ", "
			<< "Hedge Flag: " << trade->HedgeFlag << ", "
			<< "Price: " << trade->Price << ", "
			<< "Volume:" << trade->Volume << ", "
			<< "Order Ref: " << trade->OrderRef << ", "
			<< "Order Local ID: " << trade->OrderLocalID << ", "
			<< "Trade Time: " << trade->TradeTime << ", "
			<< "Business Unit: " << trade->BusinessUnit << ", "
			<< "Sequence Number: " << trade->SequenceNo << ", "
			<< "Broker Order Sequence: " << trade->BrokerOrderSeq << ", "
			<< "Trading Day: " << trade->TradingDay << "]" << endl;
	}

	cerr << "-------------Finished Printing TradeList----------" << endl;
}

void TdSpi::ShowPositionDetail()
{
	cerr << "--------------Start Printing Not Closed Long Position Detail-----------" << endl;
	list<CThostFtdcTradeField*> _positionDetail_NotClosed_Long = GetPositionDetailList_NotClosed_Long();
	for (CThostFtdcTradeField* trade : _positionDetail_NotClosed_Long)
	{
		cerr << "[Investor ID: " << trade->InvestorID << ", "
			<< "User ID: " << trade->UserID << ", "
			<< "Trade ID: " << trade->TradeID << ", "
			<< "Instrument ID: " << trade->InstrumentID << ", "
			<< "Direction: " << trade->Direction << ", "
			<< "Offset Flag: " << trade->OffsetFlag << ", "
			<< "Hedge Flag: " << trade->HedgeFlag << ", "
			<< "Price: " << trade->Price << ", "
			<< "Volume:" << trade->Volume << ", "
			<< "Order Ref: " << trade->OrderRef << ", "
			<< "Order Local ID: " << trade->OrderLocalID << ", "
			<< "Trade Time: " << trade->TradeTime << ", "
			<< "Business Unit: " << trade->BusinessUnit << ", "
			<< "Sequence Number: " << trade->SequenceNo << ", "
			<< "Broker Order Sequence: " << trade->BrokerOrderSeq << ", "
			<< "Trading Day: " << trade->TradingDay << "]" << endl;
	}

	cerr << "-------------Finished Printing Not Closed Long Position Detail----------" << endl;

	cerr << "--------------Start Printing Not Closed Short Position Detail-----------" << endl;
	list<CThostFtdcTradeField*> _positionDetail_NotClosed_Short = GetPositionDetailList_NotClosed_Short();
	for (CThostFtdcTradeField* trade : _positionDetail_NotClosed_Short)
	{
		cerr << "[Investor ID: " << trade->InvestorID << ", "
			<< "User ID: " << trade->UserID << ", "
			<< "Trade ID: " << trade->TradeID << ", "
			<< "Instrument ID: " << trade->InstrumentID << ", "
			<< "Direction: " << trade->Direction << ", "
			<< "Offset Flag: " << trade->OffsetFlag << ", "
			<< "Hedge Flag: " << trade->HedgeFlag << ", "
			<< "Price: " << trade->Price << ", "
			<< "Volume:" << trade->Volume << ", "
			<< "Order Ref: " << trade->OrderRef << ", "
			<< "Order Local ID: " << trade->OrderLocalID << ", "
			<< "Trade Time: " << trade->TradeTime << ", "
			<< "Business Unit: " << trade->BusinessUnit << ", "
			<< "Sequence Number: " << trade->SequenceNo << ", "
			<< "Broker Order Sequence: " << trade->BrokerOrderSeq << ", "
			<< "Trading Day: " << trade->TradingDay << "]" << endl;
	}

	cerr << "-------------Finished Printing Not Closed Short Position Detail----------" << endl;
}

CThostFtdcTradeField* TdSpi::GetTrade(CThostFtdcTradeField* pTrade, vector<CThostFtdcTradeField*> trades)
{
	for (CThostFtdcTradeField* trade : trades)
	{
		if (!strcmp(trade->ExchangeID, pTrade->ExchangeID) &&
			!strcmp(trade->TradeID, pTrade->TradeID) &&
			trade->Direction == pTrade->Direction)
		{
			return trade;
		}
	}

	return nullptr;
}

std::unordered_map<int, CThostFtdcOrderField*> TdSpi::GetOrderMap()
{
	lock_guard<mutex> m_lock(m_mutex);
	return orderMap;
}

std::unordered_map<int, CThostFtdcOrderField*> TdSpi::GetmOrders()
{
	lock_guard<mutex> m_lock(m_mutex);
	return m_Orders;
}

void TdSpi::addOrderToOrderMap(CThostFtdcOrderField* pOrder)
{
	lock_guard<mutex> m_lock(m_mutex);//加锁，保证这个set数据的安全
	CThostFtdcOrderField* order = new CThostFtdcOrderField();//创建order
	memcpy(order, pOrder, sizeof(CThostFtdcOrderField));//pOrder复制给order
	orderMap[pOrder->BrokerOrderSeq] = order;
}

std::vector<CThostFtdcTradeField*> TdSpi::GetTradeList()
{
	lock_guard<mutex> m_lock(m_mutex);
	return tradeList;
}

std::vector<CThostFtdcTradeField*> TdSpi::GetmTrades()
{
	lock_guard<mutex> m_lock(m_mutex);
	return m_Trades;
}

std::list<CThostFtdcTradeField*> TdSpi::GetPositionDetailList_NotClosed_Long()
{
	lock_guard<mutex> m_lock(m_mutex);
	return positionDetailList_NotClosed_Long;
}

std::list<CThostFtdcTradeField*> TdSpi::GetPositionDetailList_NotClosed_Short()
{
	lock_guard<mutex> m_lock(m_mutex);
	return positionDetailList_NotClosed_Short;
}

void TdSpi::addTradeToList(CThostFtdcTradeField *pTrade, vector<CThostFtdcTradeField*>& trades)
{
	lock_guard<mutex> m_lock(m_mutex);//加锁，保证这个set数据的安全
	CThostFtdcTradeField *trade = new CThostFtdcTradeField();//创建trade
	memcpy(trade, pTrade, sizeof(CThostFtdcTradeField));//pTrade复制给trade
	trades.push_back(trade);
}

bool TdSpi::updatePositionDetail(CThostFtdcTradeField* pTrade)
{
	if (!pTrade)
	{
		return false;
	}
	
	lock_guard<std::mutex>m_lock(m_mutex);//加锁，保证这个映射数据的安全
	
	// 如果是开仓，则插入trade为新的持仓明细
	if (pTrade->TradeID && pTrade->OffsetFlag == THOST_FTDC_OF_Open)
	{
		CThostFtdcTradeField* trade = new CThostFtdcTradeField(); //创建CThostFtdcTradeField*
		memcpy(trade, pTrade, sizeof(CThostFtdcTradeField));
		strcpy(trade->TradeDate, m_cTradingDay); // 对模拟环境可能是错误的时间
		if (pTrade->Direction == THOST_FTDC_D_Buy)
		{
			positionDetailList_NotClosed_Long.push_back(trade);
		}
		else if (pTrade->Direction == THOST_FTDC_D_Sell)
		{
			positionDetailList_NotClosed_Short.push_back(trade);
		}
	}
	else //如果是平仓则删除对应的持仓明细
	{
		// 成交的手数
		int vol = pTrade->Volume;
		list<CThostFtdcTradeField*> *pos = nullptr;
		if (pTrade->Direction == THOST_FTDC_D_Buy) // 买入平仓，找寻空单的持仓明细
		{
			pos = &positionDetailList_NotClosed_Short;
		}
		else if (pTrade->Direction == THOST_FTDC_D_Sell) // 卖出平仓，找寻多单的持仓明细
		{
			pos = &positionDetailList_NotClosed_Long;
		}
		
		if (!pos)
		{
			cerr << "[TdSpi]: Empty targeted position list to close out the position" << endl;
			return false;
		}

		int j = 0; // 用来加总符合条件的
		bool findflag = false;
		
		// 是否是平今
		bool closeTd = pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday && (!strcmp(pTrade->ExchangeID, exchangeIDSHFE) || !strcmp(pTrade->ExchangeID, exchangeIDINE));
		double avgClosedOpenPrice = 0.0;
		for (auto it = (*pos).begin(); it != (*pos).end(); ++it)
		{
			if (strcmp((*it)->InstrumentID, pTrade->InstrumentID))
			{
				continue;
			}

			if (closeTd) //平今，则需要查找持仓明细中第一笔当日开仓的持仓
			{
				if (!findflag && !strcmp((*it)->TradeDate, m_cTradingDay))
				{
					findflag = true;
				}

				if (findflag && strcmp((*it)->TradeDate, m_cTradingDay) == 0)
				{
					int temp = j;
					// 判断第i个持仓明细的手数是否足够vol - temp
					j = temp + ((*it)->Volume > (vol - temp) ? (vol - temp) : (*it)->Volume);
					(*it)->Volume -= (j - temp);
					avgClosedOpenPrice += (*it)->Price * (j - temp);
				}
			}
			else //平仓
			{
				//上期所或者能源所平昨
				if (!strcmp(pTrade->ExchangeID, exchangeIDSHFE) || !strcmp(pTrade->ExchangeID, exchangeIDINE))
				{
					if (!findflag && strcmp((*it)->TradeDate, m_cTradingDay))
					{
						findflag = true;
					}

					if (findflag && strcmp((*it)->TradeDate, m_cTradingDay))
					{
						int temp = j;
						//判断第i个持仓明细的手数是否足够vol-temp
						j = temp + ((*it)->Volume > (vol - temp) ? (vol - temp) : (*it)->Volume);
						(*it)->Volume -= (j - temp);
						avgClosedOpenPrice += (*it)->Price * (j - temp);
					}
				}
				else //中金所、大商所、郑商所平仓
				{
					int temp = j;
					j = temp + ((*it)->Volume > (vol - temp) ? (vol - temp) : (*it)->Volume); (*it)->Volume -= (j - temp);
					avgClosedOpenPrice += (*it)->Price * (j - temp);
				}
			}

			if (j == vol)
			{
				break;
			}
		}

		//删除持仓量为0的持仓明细
		for (auto it = (*pos).begin(); it != (*pos).end();)
		{
			if ((*it)->Volume == 0)
			{
				it = (*pos).erase(it); // erase 返回it后面的一个迭代器
			}
			else
			{
				++it;
			}
		}

		// 计算平仓盈亏
		string instrumentID(pTrade->InstrumentID);
		auto it = m_position_field_map.find(instrumentID);
		if (it == m_position_field_map.end())
		{
			cerr << "[TdSpi]: The closed instrumentID is not in the m_position_field_map, something goes wrong" << endl;
			return false;
		}

		auto instrIt = m_inst_field_map.find(instrumentID);
		if (instrIt == m_inst_field_map.end())
		{
			cerr << "[TdSpi]: The closed instrumentID is not in the m_inst_field_map, something goes wrong" << endl;
			return false;
		}

		int contractSize = instrIt->second->VolumeMultiple;
		avgClosedOpenPrice /= vol;
		if (pTrade->Direction == THOST_FTDC_D_Buy)
		{
			// 买多平仓是平空单
			it->second->ShortCloseProfit += (avgClosedOpenPrice - pTrade->Price) * contractSize * vol;
		}
		else
		{
			// 卖空平仓是平多单
			it->second->LongCloseProfit += (pTrade->Price - avgClosedOpenPrice) * contractSize * vol;
		}
		
	}
		
	return true;
}

void TdSpi::updatePositionFieldMap(TThostFtdcInstrumentIDType instrumentID)
{
	lock_guard<std::mutex>m_lock(m_mutex);//加锁，保证这个更新数据的安全
	if (m_position_field_map.find(instrumentID) != m_position_field_map.end())
	{
		return;
	}

	cerr << "[TdSpi]: Insert new instrument to position field map: " << instrumentID << endl;
	position_field *newPosition = new position_field();
	newPosition->instId = instrumentID;
	m_position_field_map[instrumentID] = newPosition;
}

void TdSpi::updatePosition(string instrumentID)
{
	// 浮动盈亏
	double openProfit = 0.0;
	// 持仓盈亏
	double positionProfit = 0.0;
	// 平仓盈亏 
	double closeProfit = 0.0;

	lock_guard<mutex> m_lock(m_mutex);//加锁，保证这个更新数据的安全
	for (auto it : m_position_field_map)
	{
		if (!instrumentID.length() || instrumentID == it.second->instId)
		{
			refreshPositionField(it.first, it.second);
		}

		openProfit += it.second->LongOpenProfit + it.second->ShortOpenProfit;
		positionProfit += it.second->LongPositionProfit + it.second->ShortPositionProfit;
		closeProfit += it.second->LongCloseProfit + it.second->ShortCloseProfit;
	}

	m_OpenProfit = openProfit;
	m_PositionProfit = positionProfit;
	m_CloseProfit = closeProfit;
	cerr << "[TdSpii]: Open Profit: " << m_OpenProfit << ", Position Profit: " << m_PositionProfit << ", Close Profit: " << m_CloseProfit << endl;
}

void TdSpi::updatePositionMDPrice(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	static int tickCount = 0;

	if (pDepthMarketData->PreSettlementPrice < 0.0001 || pDepthMarketData->LastPrice < 0.0001)
	{
		return;
	}

	if (pDepthMarketData->LastPrice < pDepthMarketData->LowerLimitPrice || pDepthMarketData->LastPrice > pDepthMarketData->UpperLimitPrice)
	{
		return;
	}

	string instrumentID = pDepthMarketData->InstrumentID;
	
	{
		lock_guard<mutex> m_lock(m_mutex);
		auto it = m_position_field_map.find(instrumentID);
		if (it == m_position_field_map.end())
		{
			return;
		}

		if (it->second->PreSettlementPrice < 0.0001)
		{
			it->second->PreSettlementPrice = pDepthMarketData->PreSettlementPrice;
		}

		it->second->lastPrice = pDepthMarketData->LastPrice;
	}

	updatePosition(instrumentID);

	if (tickCount++ % 20 == 0)
	{
		ShowPosition();
	}
}

void TdSpi::refreshPositionField(std::string instrumentID, position_field* positionField)
{
	position_field newPositionField;

	// 多单查找
	for (CThostFtdcTradeField* field : positionDetailList_NotClosed_Long)
	{
		if (strcmp(field->InstrumentID, instrumentID.c_str()))
		{
			continue;
		}

		newPositionField.LongPosition += field->Volume; // 加总持仓手数

		// 加总今仓手数
		if (!strcmp(m_cTradingDay, field->TradeDate) || !strcmp(m_cTradingDay, field->TradingDay)) // 某些交易所支持的交易天不一样
		{
			newPositionField.TodayLongPosition += field->Volume;
			newPositionField.LongAvgHoldingPrice += field->Price * field->Volume;
		}
		else // 加总昨仓手数
		{
			newPositionField.YdLongPosition += field->Volume;
			newPositionField.LongAvgHoldingPrice += positionField->PreSettlementPrice * field->Volume;
		}

		newPositionField.LongAvgEntryPrice += field->Price * field->Volume;
	}

	// 如果多头仓位不为0，可以计算两个均价
	if (newPositionField.LongPosition)
	{
		newPositionField.LongAvgHoldingPrice /= newPositionField.LongPosition;
		newPositionField.LongAvgEntryPrice /= newPositionField.LongPosition;

		// 计算持仓盈亏和浮动盈亏
		int multiple = 0;
		if (m_inst_field_map.find(instrumentID) != m_inst_field_map.end())
		{
			multiple = m_inst_field_map[instrumentID]->VolumeMultiple;
			// 计算浮动开仓盈亏
			newPositionField.LongOpenProfit = (positionField->lastPrice - newPositionField.LongAvgEntryPrice) * multiple * newPositionField.LongPosition;
			// 计算持仓盈亏
			newPositionField.LongPositionProfit = (positionField->lastPrice - newPositionField.LongAvgHoldingPrice) * multiple * newPositionField.LongPosition;
		}
	}

	positionField->LongPosition = newPositionField.LongPosition;
	positionField->TodayLongPosition = newPositionField.TodayLongPosition;
	positionField->YdLongPosition = newPositionField.YdLongPosition;
	positionField->LongAvgEntryPrice = newPositionField.LongAvgEntryPrice;
	positionField->LongAvgHoldingPrice = newPositionField.LongAvgHoldingPrice;
	positionField->LongOpenProfit = newPositionField.LongOpenProfit;
	positionField->LongPositionProfit = newPositionField.LongPositionProfit;

	// 空单查找
	for (CThostFtdcTradeField* field : positionDetailList_NotClosed_Short)
	{
		if (strcmp(field->InstrumentID, instrumentID.c_str()))
		{
			continue;
		}

		newPositionField.ShortPosition += field->Volume; // 加总持仓手数

		// 加总今仓手数
		if (!strcmp(m_cTradingDay, field->TradeDate) || !strcmp(m_cTradingDay, field->TradingDay)) // 某些交易所支持的交易天不一样
		{
			newPositionField.TodayShortPosition += field->Volume;
			newPositionField.ShortAvgHoldingPrice += field->Price * field->Volume;
		}
		else // 加总昨仓手数
		{
			newPositionField.YdShortPosition += field->Volume;
			newPositionField.ShortAvgHoldingPrice += positionField->PreSettlementPrice * field->Volume;
		}

		newPositionField.ShortAvgEntryPrice += field->Price * field->Volume;
	}

	// 如果空头仓位不为0，可以计算两个均价
	if (newPositionField.ShortPosition)
	{
		newPositionField.ShortAvgHoldingPrice /= newPositionField.ShortPosition;
		newPositionField.ShortAvgEntryPrice /= newPositionField.ShortPosition;

		// 计算持仓盈亏和浮动盈亏
		int multiple = 0;
		if (m_inst_field_map.find(instrumentID) != m_inst_field_map.end())
		{
			multiple = m_inst_field_map[instrumentID]->VolumeMultiple;
			// 计算浮动开仓盈亏
			newPositionField.ShortOpenProfit = (newPositionField.ShortAvgEntryPrice - positionField->lastPrice) * multiple * newPositionField.ShortPosition;
			// 计算持仓盈亏
			newPositionField.ShortPositionProfit = (newPositionField.ShortAvgHoldingPrice - positionField->lastPrice) * multiple * newPositionField.ShortPosition;
		}
	}

	positionField->ShortPosition = newPositionField.ShortPosition;
	positionField->TodayShortPosition = newPositionField.TodayShortPosition;
	positionField->YdShortPosition = newPositionField.YdShortPosition;
	positionField->ShortAvgEntryPrice = newPositionField.ShortAvgEntryPrice;
	positionField->ShortAvgHoldingPrice = newPositionField.ShortAvgHoldingPrice;
	positionField->ShortOpenProfit = newPositionField.ShortOpenProfit;
	positionField->ShortPositionProfit = newPositionField.ShortPositionProfit;
}

bool TdSpi::isMDPriceUpdated()
{
	lock_guard<mutex> m_lock(m_mutex);
	for (auto it : m_position_field_map)
	{
		// 如果有更新，last price和pre settlement price都会大于0
		if (it.second->lastPrice < 0.0001 || it.second->PreSettlementPrice < 0.0001)
		{
			return false;
		}
	}

	return true;
}

