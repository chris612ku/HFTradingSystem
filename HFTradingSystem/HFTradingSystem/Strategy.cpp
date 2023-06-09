#include "Strategy.h"
#include "KData.h"
#include "timer.h"

using namespace std;

extern mutex m_mutex;

void Strategy::OnTick(CThostFtdcDepthMarketDataField* pDepthMD)
{
	if (!pDepthMD)
	{
		cerr << "[Strategy]: passed-in pDepthMD is nullptr" << endl;
		return;
	}

	if (strcmp(m_instId, pDepthMD->InstrumentID))
	{
		printf("[Strategy]: the passed-in depth market data is not the instrument ID we care: (%s, %s)\n", m_instId, pDepthMD->InstrumentID);
		return;
	}
	
	// 记录深度行情数据
	UpdateDepthMDList(pDepthMD);

	/*UpdateSums(pDepthMD);
	EvaluateBuySellSignal(pDepthMD);*/

	// 计算买卖信号并发单
	CalculateBuySellSignal(pDepthMD);
}

void Strategy::OnStrategyStart()
{
	tickCount = 0;
	UpdatePositionFieldStgy();
	m_posfield.instId = m_instId;
	m_fMinMove = (m_instField_map.find(m_instId) != m_instField_map.end()) ? m_instField_map[m_instId]->PriceTick : 0;
	
	// 买卖手数
	m_nVolume = 1;

	// 记录开仓委托
	setM_pOpenOrder(nullptr);
	
	// 记录平仓委托
	setM_pCloseOrder(nullptr);

	es1 = ESignal::NoAction;
	es2 = ESignal::NoAction;

	last5Ticks = vector<double>(5, 0.0);
	last20Ticks = vector<double>(20, 0.0);

	currentSum5 = 0.0;
	currentSum20 = 0.0;
}

void Strategy::OnStrategyEnd()
{
}

void Strategy::OnBar(CKBar *pBar, CKBarSeries* pSeries)
{
}

void Strategy::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	if (!pOrder)
	{
		cerr << "[Strategy]: passed-in pOrder is nullptr" << endl;
		return;
	}

	if (strcmp(m_instId, pOrder->InstrumentID))
	{
		printf("[Strategy]: the passed-in depth market data is not the instrument ID we care: (%s, %s)\n", m_instId, pOrder->InstrumentID);
		return;
	}

	// 如果是撤单
	if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
	{
		// 代表开仓
		if (pOrder->CombOffsetFlag[0] == '0')
		{
			setM_pOpenOrder(nullptr);
		}
		else
		{
			setM_pCloseOrder(nullptr);

			// 止损
			if (m_pDepthMDList.size() > 0)
			{
				CThostFtdcDepthMarketDataField* pDepthMD = m_pDepthMDList.back();

				// 多单止损
				if (m_posfield.LongPosition)
				{
					setM_pCloseOrder(Sell(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1 - 2 * m_fMinMove));
					RegisterTimer(300, 11, getM_pCloseOrder());
				}

				// 空单止损
				if (m_posfield.ShortPosition)
				{
					setM_pCloseOrder(BuytoCover(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1 + 2 * m_fMinMove));
					RegisterTimer(300, 11, getM_pCloseOrder());
				}
			}
		}
	}
}

void Strategy::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder)
{
	// 代表开仓
	if (pInputOrder->CombOffsetFlag[0] == '0')
	{
		setM_pOpenOrder(nullptr);
	}
	else
	{
		setM_pCloseOrder(nullptr);
	}
}

void Strategy::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	if (!pTrade)
	{
		cerr << "[Strategy]: the passed-in pTrade is nullptr" << endl;
		return;
	}

	if (strcmp(pTrade->InstrumentID, m_instId))
	{
		printf("[Strategy]: the passed-in depth market data is not the instrument ID we care: (%s, %s)\n", m_instId, pTrade->InstrumentID);
		return;
	}

	UpdatePositionFieldStgy();

	// 如果是开仓
	if (pTrade->OffsetFlag == '0')
	{
		nStartSeconds = GetLocalTimeSeconds();
	}
	else
	{
		if (!m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			setM_pOpenOrder(nullptr);
			setM_pCloseOrder(nullptr);
			nEndSeconds = 0;
			nStartSeconds = 0;
		}
	}

	// 如果是开仓,设置止盈单
	if (pTrade->OffsetFlag == '0')
	{
		if (m_pOpenOrder && pTrade->Direction == THOST_FTDC_D_Buy)
		{
			double price = pTrade->Price + 3 * m_fMinMove;
			setM_pOpenOrder(nullptr);
			setM_pCloseOrder(Sell(pTrade->InstrumentID, pTrade->ExchangeID, m_nVolume, price));
		}
		else if (m_pOpenOrder && pTrade->Direction == THOST_FTDC_D_Sell)
		{
			double price = pTrade->Price - 3 * m_fMinMove;
			setM_pOpenOrder(nullptr);
			setM_pCloseOrder(BuytoCover(pTrade->InstrumentID, pTrade->ExchangeID, m_nVolume, price));
		}
	}
	else if (es2 != ESignal::NoAction) // 该笔交易为平仓。则需要判断是否需要反手开仓
	{
		if (m_pDepthMDList.size())
		{
			CThostFtdcDepthMarketDataField* pDepthMD = m_pDepthMDList.back();
			if (pTrade->Direction == THOST_FTDC_D_Buy && es2 == ESignal::BuyOpen)
			{
				setM_pCloseOrder(nullptr);
				setM_pOpenOrder(Buy(pTrade->InstrumentID, pTrade->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
				RegisterTimer(1000, 11, getM_pOpenOrder());
			}
			else if (pTrade->Direction == THOST_FTDC_D_Sell && es2 == ESignal::SellOpen)
			{
				setM_pCloseOrder(nullptr);
				setM_pOpenOrder(Short(pTrade->InstrumentID, pTrade->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
				RegisterTimer(1000, 11, getM_pOpenOrder());
			}
		}
	}
}

void Strategy::CancelOrder(CThostFtdcOrderField* pOrder)
{
	if (!pOrder)
	{
		cerr << "[Strategy]: Cannot cancel order due to nullptr pOrder" << endl;
		return;
	}

	if (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded && pOrder->OrderStatus != THOST_FTDC_OST_Canceled)
	{
		CThostFtdcOrderField* order = m_pUserTDSpi_stgy->GetOrder(pOrder);
		m_pUserTDSpi_stgy->CancelOrder(order);
	}
}

void Strategy::RegisterTimer(int milliSeconds, int nAction, CThostFtdcOrderField* pOrder)
{
	Timer t1;
	t1.startOnce(milliSeconds, bind(&Strategy::OnTimer, this, pOrder, nAction));
}

void Strategy::OnTimer(CThostFtdcOrderField* pOrder, long lData)
{
	cerr << "[Strategy]: Timer triggered" << endl;

	CThostFtdcOrderField* currentM_pOpenOrder = getM_pOpenOrder();
	CThostFtdcOrderField* currentM_pCloseOrder = getM_pCloseOrder();
	if ((currentM_pOpenOrder && !strcmp(currentM_pOpenOrder->OrderRef, pOrder->OrderRef)) ||
		(currentM_pCloseOrder && !strcmp(currentM_pCloseOrder->OrderRef, pOrder->OrderRef)))
	{
		CancelOrder(pOrder);
	}
}

CThostFtdcOrderField* Strategy::Buy(const char* InstrumentID, const char* ExchangeID, int nVolume, double fPrice)
{
	return m_pUserTDSpi_stgy->PlaceOrder(InstrumentID, ExchangeID, THOST_FTDC_D_Buy, 0, nVolume, fPrice);
}

CThostFtdcOrderField* Strategy::Sell(const char* InstrumentID, const char* ExchangeID, int nVolume, double fPrice, int YdorToday)
{
	return m_pUserTDSpi_stgy->PlaceOrder(InstrumentID, ExchangeID, THOST_FTDC_D_Sell, YdorToday, nVolume, fPrice);
}

CThostFtdcOrderField* Strategy::Short(const char* InstrumentID, const char* ExchangeID, int nVolume, double fPrice)
{
	return m_pUserTDSpi_stgy->PlaceOrder(InstrumentID, ExchangeID, THOST_FTDC_D_Sell, 0, nVolume, fPrice);
}

CThostFtdcOrderField* Strategy::BuytoCover(const char* InstrumentID, const char* ExchangeID, int nVolume, double fPrice, int YdorToday)
{
	return m_pUserTDSpi_stgy->PlaceOrder(InstrumentID, ExchangeID, THOST_FTDC_D_Buy, YdorToday, nVolume, fPrice);
}

void Strategy::set_instPostion_map_stgy(unordered_map<string, CThostFtdcInstrumentField*> inst_map)
{
	auto it = inst_map.find(m_instId);
	if (it == inst_map.end())
	{
		printf("The passed-in inst_map does not contain the instrument ID we want: %s\n", m_instId);
		return;
	}

	auto it2 = m_instField_map.find(m_instId);
	if (it2 != m_instField_map.end())
	{
		return;
	}

	m_instField_map[m_instId] = inst_map[m_instId];
}

void Strategy::UpdatePositionFieldStgy()
{
	position_field* posField = GetPositionField(m_instId);
	if (posField)
	{
		m_posfield = *posField;
	}
}

position_field* Strategy::GetPositionField(std::string instId)
{
	auto it = m_pUserTDSpi_stgy->m_position_field_map.find(instId);
	if (it == m_pUserTDSpi_stgy->m_position_field_map.end())
	{
		return nullptr;
	}
	
	return it->second;
}

void Strategy::UpdateDepthMDList(CThostFtdcDepthMarketDataField* pDepth)
{
	if (!pDepth)
	{
		cerr << "[Strategy]: passed-in pDepth is nullptr" << endl;
		return;
	}

	m_pDepthMDList.push_back(pDepth);
	if (m_pDepthMDList.size() > 30)
	{
		m_pDepthMDList.pop_front();
	}
}

void Strategy::CalculateProfitInfo(CThostFtdcDepthMarketDataField* pDepthMD)
{
}

void Strategy::SaveTickToVec(CThostFtdcDepthMarketDataField* pDepthMD)
{
}

void Strategy::SaveTickToTxtCsv(CThostFtdcDepthMarketDataField* pDepthMD)
{
}

void Strategy::CalculateBuySellSignal(CThostFtdcDepthMarketDataField* pDepthMD)
{
	// 行情是否为空
	if (!pDepthMD)
	{
		cerr << "[Strategy]: the passed-in pDepthMD is nullptr" << endl;
		return;
	}

	es1 = ESignal::NoAction;
	es2 = ESignal::NoAction;

	// 公平价格
	double midPrice = (pDepthMD->BidPrice1 * pDepthMD->AskVolume1 + pDepthMD->AskPrice1 * pDepthMD->BidVolume1) / (pDepthMD->BidVolume1 + pDepthMD->AskVolume1);

	// 表示开平
	int nOpenClose = -1;

	// 买卖信号0，1代表买入，-1代表卖出
	int nSignal = 0;

	// 开仓
	if (!m_posfield.LongPosition && midPrice - pDepthMD->LastPrice >= 1 * m_fMinMove)
	{
		nSignal = 1;
	}
	else if (!m_posfield.ShortPosition && pDepthMD->LastPrice - midPrice >= 1 * m_fMinMove)
	{
		nSignal = -1;
	}

	// 信号做多
	if (nSignal > 0)
	{
		if (!getM_pOpenOrder() && !getM_pCloseOrder() && !m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			setM_pOpenOrder(Buy(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
			RegisterTimer(1000, 11, getM_pOpenOrder());
		}
		else if (!getM_pCloseOrder() && !m_posfield.LongPosition && m_posfield.ShortPosition)
		{
			es1 = ESignal::BuyToCoverTD;
			es2 = ESignal::BuyOpen;
			setM_pCloseOrder(BuytoCover(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
			RegisterTimer(1000, 11, getM_pCloseOrder());
		}
	}
	else if (nSignal < 0) // 信号做空
	{
		if (!getM_pOpenOrder() && !getM_pCloseOrder() && !m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			setM_pOpenOrder(Short(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1));
			RegisterTimer(1000, 11, getM_pOpenOrder());
		}
		else if (!getM_pCloseOrder() && m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			es1 = ESignal::SellTD;
			es2 = ESignal::SellOpen;
			setM_pCloseOrder(Sell(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1));
			RegisterTimer(1000, 11, getM_pCloseOrder());
		}	
	}

	// 止损
	if (m_posfield.LongPosition || m_posfield.ShortPosition)
	{
		nEndSeconds = GetLocalTimeSeconds();
		bool bPriceStopLoss = false;
		bool bTimeStop = false;
		if (m_posfield.LongPosition)
		{
			if (m_posfield.LongAvgEntryPrice - midPrice >= 5 * m_fMinMove)
			{
				bPriceStopLoss = true;
			}

			if (nEndSeconds - nStartSeconds >= 60)
			{
				bTimeStop = true;
			}

			if (bPriceStopLoss || bTimeStop)
			{
				es1 = ESignal::SellTD;
				if (m_pCloseOrder)
				{
					CancelOrder(m_pCloseOrder);
					return;
				}
				else
				{
					setM_pOpenOrder(nullptr);
					setM_pCloseOrder(Sell (pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1));
					RegisterTimer(300, 11, getM_pCloseOrder());
				}
			}
		}

		if (m_posfield.ShortPosition)
		{
			if (midPrice - m_posfield.ShortAvgEntryPrice >= 5 * m_fMinMove)
			{
				bPriceStopLoss = true;
			}

			if (nEndSeconds - nStartSeconds >= 60)
			{
				bTimeStop = true;
			}

			if (bPriceStopLoss || bTimeStop)
			{
				es1 = ESignal::BuyToCoverTD;
				CThostFtdcOrderField* m_pCloseOrder = getM_pCloseOrder();
				if (m_pCloseOrder)
				{
					CancelOrder(m_pCloseOrder);
					return;
				}
				else
				{
					setM_pOpenOrder(nullptr);
					setM_pCloseOrder(BuytoCover(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
					RegisterTimer(300, 11, getM_pCloseOrder());
				}
			}
		}
	}
}

void Strategy::UpdateSums(CThostFtdcDepthMarketDataField* pDepthMD)
{
	lock_guard<mutex> m_lock(m_mutex);
	++tickCount;
	if (tickCount > 5)
	{
		currentSum5 -= last5Ticks[(tickCount - 1) % 5];
	}

	currentSum5 += pDepthMD->LastPrice;
	last5Ticks[(tickCount - 1) % 5] = pDepthMD->LastPrice;

	if (tickCount > 20)
	{
		currentSum20 -= last20Ticks[(tickCount - 1) % 20];
	}

	currentSum20 += pDepthMD->LastPrice;
	last20Ticks[(tickCount - 1) % 20] = pDepthMD->LastPrice;
}

void Strategy::EvaluateBuySellSignal(CThostFtdcDepthMarketDataField* pDepthMD)
{
	if (tickCount < 30)
	{
		return;
	}

	es1 = ESignal::NoAction;
	es2 = ESignal::NoAction;

	double last5Avg = currentSum5 / 5;
	double last20Avg = currentSum20 / 20;

	// 表示开平
	int nOpenClose = -1;

	// 买卖信号0，1代表买入，-1代表卖出
	int nSignal = 0;

	// 开仓
	if (!m_posfield.LongPosition && last5Avg > last20Avg)
	{
		nSignal = 1;
	}
	else if (!m_posfield.ShortPosition && last5Avg < last20Avg)
	{
		nSignal = -1;
	}

	// 信号做多
	if (nSignal > 0)
	{
		if (!getM_pOpenOrder() && !getM_pCloseOrder() && !m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			setM_pOpenOrder(Buy(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
			RegisterTimer(1000, 11, getM_pOpenOrder());
		}
		else if (!getM_pCloseOrder() && !m_posfield.LongPosition && m_posfield.ShortPosition)
		{
			es1 = ESignal::BuyToCoverTD;
			es2 = ESignal::BuyOpen;
			setM_pCloseOrder(BuytoCover(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
			RegisterTimer(1000, 11, getM_pCloseOrder());
		}
	}
	else if (nSignal < 0) // 信号做空
	{
		if (!getM_pOpenOrder() && !getM_pCloseOrder() && !m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			setM_pOpenOrder(Short(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1));
			RegisterTimer(1000, 11, getM_pOpenOrder());
		}
		else if (!getM_pCloseOrder() && m_posfield.LongPosition && !m_posfield.ShortPosition)
		{
			es1 = ESignal::SellTD;
			es2 = ESignal::SellOpen;
			setM_pCloseOrder(Sell(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1));
			RegisterTimer(1000, 11, getM_pCloseOrder());
		}
	}

	// 止损
	/*if (m_posfield.LongPosition || m_posfield.ShortPosition)
	{
		nEndSeconds = GetLocalTimeSeconds();
		bool bPriceStopLoss = false;
		bool bTimeStop = false;
		if (m_posfield.LongPosition)
		{
			if (m_posfield.LongAvgEntryPrice - midPrice >= 5 * m_fMinMove)
			{
				bPriceStopLoss = true;
			}

			if (nEndSeconds - nStartSeconds >= 60)
			{
				bTimeStop = true;
			}

			if (bPriceStopLoss || bTimeStop)
			{
				es1 = ESignal::SellTD;
				if (m_pCloseOrder)
				{
					CancelOrder(m_pCloseOrder);
					return;
				}
				else
				{
					setM_pOpenOrder(nullptr);
					setM_pCloseOrder(Sell(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->BidPrice1));
					RegisterTimer(300, 11, getM_pCloseOrder());
				}
			}
		}

		if (m_posfield.ShortPosition)
		{
			if (midPrice - m_posfield.ShortAvgEntryPrice >= 5 * m_fMinMove)
			{
				bPriceStopLoss = true;
			}

			if (nEndSeconds - nStartSeconds >= 60)
			{
				bTimeStop = true;
			}

			if (bPriceStopLoss || bTimeStop)
			{
				es1 = ESignal::BuyToCoverTD;
				CThostFtdcOrderField* m_pCloseOrder = getM_pCloseOrder();
				if (m_pCloseOrder)
				{
					CancelOrder(m_pCloseOrder);
					return;
				}
				else
				{
					setM_pOpenOrder(nullptr);
					setM_pCloseOrder(BuytoCover(pDepthMD->InstrumentID, pDepthMD->ExchangeID, m_nVolume, pDepthMD->AskPrice1));
					RegisterTimer(300, 11, getM_pCloseOrder());
				}
			}
		}
	}*/
}

void Strategy::setM_pOpenOrder(CThostFtdcOrderField* newM_pOpenOrder)
{
	lock_guard<mutex> m_lock(m_mutex);
	m_pOpenOrder = newM_pOpenOrder;
}

CThostFtdcOrderField* Strategy::getM_pOpenOrder()
{
	lock_guard<mutex> m_lock(m_mutex);
	return m_pOpenOrder;
}

void Strategy::setM_pCloseOrder(CThostFtdcOrderField* newM_pCloseOrder)
{
	lock_guard<mutex> m_lock(m_mutex);
	m_pCloseOrder = newM_pCloseOrder;
}

CThostFtdcOrderField* Strategy::getM_pCloseOrder()
{
	lock_guard<mutex> m_lock(m_mutex);
	return m_pCloseOrder;
}
