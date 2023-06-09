#pragma once
#include "TdSpi.h"
#include "mystruct.h"

#include <vector>

//保存账户信息的map
extern std::unordered_map<std::string, std::string> accountConfig_map;

class CKBar;
class CKBarSeries;


class Strategy { 
public:
	Strategy(TdSpi *pUserSpi_trade) : m_pUserTDSpi_stgy(pUserSpi_trade) {
		strcpy_s(m_instId, accountConfig_map["contract"].c_str()); 
		tickCount = 0;
	}
	
	// tick事件
	void OnTick(CThostFtdcDepthMarketDataField * pDepthMD);
	
	// 策略启动事件
	void OnStrategyStart();
	
	// 策略停止事件
	void OnStrategyEnd();
	
	// k线事件
	void OnBar(CKBar* pBar, CKBarSeries* pSeries);
	
	// 订单状态事件
	void OnRtnOrder(CThostFtdcOrderField *pOrder);

	// 订单被撤回
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder);
	
	// 成交事件
	void OnRtnTrade(CThostFtdcTradeField *pTrade);
	
	// 撤单操作
	void CancelOrder(CThostFtdcOrderField *pOrder);
	
	// 注册计时器，做高频交易时，需要多少毫秒以后不成交撤单，需要计时器
	void RegisterTimer(int milliSeconds, int nAction, CThostFtdcOrderField *pOrder);
	
	// 计时器通知时间到了
	void OnTimer(CThostFtdcOrderField *pOrder, long lData);
	
	// buy open
	CThostFtdcOrderField *Buy(const char *InstrumentID, const char *ExchangeID, int nVolume, double fPrice);
	
	// sell close
	CThostFtdcOrderField *Sell(const char *InstrumentID, const char* ExchangeID, int nVolume, double fPrice, int YdorToday = 2);
	
	// sell open;
	CThostFtdcOrderField *Short(const char *InstrumentID, const char* ExchangeID, int nVolume, double fPrice);
	
	// buy to close;
	CThostFtdcOrderField *BuytoCover(const char *InstrumentID, const char* ExchangeID, int nVolume, double fPrice, int YdorToday = 2);

	// 把持仓合约设置到策略里面
	void set_instPostion_map_stgy(std::unordered_map<std::string, CThostFtdcInstrumentField *> inst_map);

	void UpdatePositionFieldStgy();

	position_field* GetPositionField(std::string instId);

	void UpdateDepthMDList(CThostFtdcDepthMarketDataField* pDepth);

private:
	std::unordered_map<std::string, CThostFtdcInstrumentField *> m_instField_map;
	
	TdSpi *m_pUserTDSpi_stgy;
	
	// 计算账户盈亏
	void CalculateProfitInfo(CThostFtdcDepthMarketDataField * pDepthMD);
	
	// 保存tick数据到vector
	void SaveTickToVec(CThostFtdcDepthMarketDataField * pDepthMD);
	
	// 保存tick数据到txt和csv
	void SaveTickToTxtCsv(CThostFtdcDepthMarketDataField * pDepthMD);
	
	// 计算开仓平仓信号
	void CalculateBuySellSignal(CThostFtdcDepthMarketDataField *pDepthMD);

	void UpdateSums(CThostFtdcDepthMarketDataField* pDepthMD);

	void EvaluateBuySellSignal(CThostFtdcDepthMarketDataField* pDepthMD);
	
	// 交易的合约
	char m_instId[32];
	
	// 手数
	int m_nVolume;

	// 最小变动价位
	double m_fMinMove;

	// 开仓的委托
	CThostFtdcOrderField* m_pOpenOrder;

	// 平仓的委托
	CThostFtdcOrderField* m_pCloseOrder;

	// 开仓时间
	int nStartSeconds;

	// 平仓时间
	int nEndSeconds;

	// 交易信号
	ESignal es1;

	ESignal es2;

	unsigned int tickCount;

	position_field m_posfield;

	std::list<CThostFtdcDepthMarketDataField*> m_pDepthMDList;

	std::vector<double> last5Ticks;

	std::vector<double> last20Ticks;

	double currentSum5;

	double currentSum20;

	void setM_pOpenOrder(CThostFtdcOrderField* newM_pOpenOrder);

	CThostFtdcOrderField* getM_pOpenOrder();

	void setM_pCloseOrder(CThostFtdcOrderField* newM_pCloseOrder);

	CThostFtdcOrderField* getM_pCloseOrder();
};