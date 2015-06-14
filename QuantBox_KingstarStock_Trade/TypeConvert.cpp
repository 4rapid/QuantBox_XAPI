#include "stdafx.h"
#include "TypeConvert.h"


/// ������OpenQuant FIXһ����Ч���������򵥣�������Ҫ����ô�������
/// ����һ�����ӵ�ĳ��״̬�����������ģ��µ������ֳɽ�����ȫ�ɽ�
/// EmitAccept,EmitFill
/// OnRtnOrder,OnRtnTrade,���ת��Emit
/// EmitAccept��ʲô
///
/// �ӿ������ṩ�Ļر����Է����ַ�����ExecutionReport��ί�лر���ɽ��ر�
/// OpenQuant��ʹ��ExecutionReport��������ΪOQ�Լ���OrderManager������������Ҫ����ί�кͳɽ��б���û���õ���
/// ���Խӿ�Ӧ������ί����ɽ��ر�

//PutCall TThostFtdcOptionsTypeType_2_PutCall(TThostFtdcOptionsTypeType In)
//{
//	if (In == THOST_FTDC_CP_CallOptions)
//		return PutCall::Call;
//	return PutCall::Put;
//}
//
//HedgeFlagType TThostFtdcHedgeFlagType_2_HedgeFlagType(TThostFtdcHedgeFlagType In)
//{
//	switch (In)
//	{
//	case THOST_FTDC_HF_Arbitrage:
//		return HedgeFlagType::Arbitrage;
//	case THOST_FTDC_HF_Hedge:
//		return HedgeFlagType::Hedge;
//	case THOST_FTDC_HF_Speculation:
//	default:
//		return HedgeFlagType::Speculation;
//	}
//}
//
//TThostFtdcHedgeFlagType HedgeFlagType_2_TThostFtdcHedgeFlagType(HedgeFlagType In)
//{
//	switch (In)
//	{
//	case HedgeFlagType::Arbitrage:
//		return THOST_FTDC_HF_Arbitrage;
//	case HedgeFlagType::Hedge:
//		return THOST_FTDC_HF_Hedge;
//	case HedgeFlagType::Speculation:
//	default:
//		return THOST_FTDC_HF_Speculation;
//	}
//}
//
//OpenCloseType TThostFtdcOffsetFlagType_2_OpenCloseType(TThostFtdcOffsetFlagType In)
//{
//	switch (In)
//	{
//	case THOST_FTDC_OF_CloseToday:
//		return OpenCloseType::CloseToday;
//	case THOST_FTDC_OF_Close:
//		return OpenCloseType::Close;
//	case THOST_FTDC_OF_Open:
//	default:
//		return OpenCloseType::Open;
//	}
//}
//
//TThostFtdcOffsetFlagType OpenCloseType_2_TThostFtdcOffsetFlagType(OpenCloseType In)
//{
//	switch (In)
//	{
//	case OpenCloseType::CloseToday:
//		return THOST_FTDC_OF_CloseToday;
//	case OpenCloseType::Close:
//		return THOST_FTDC_OF_Close;
//	case OpenCloseType::Open:
//	default:
//		return THOST_FTDC_OF_Open;
//	}
//}

void OrderField_2_TBSType(OrderField* pIn, PSTOrder pOut)
{
	switch (pIn->Side)
	{
	case OrderSide::Sell:
		strcpy(pOut->bs, "1");
		break;
	case OrderSide::Buy:
		strcpy(pOut->bs, "1");
		break;
	default:
		break;
	}

	//switch (pIn->Side)
	//{
	//case OrderSide::Sell:
	//	strcpy(pOut->bs, "1");
	//case OrderSide::Buy:
	//	strcpy(pOut->bs, "1");
	//default:
	//	break;
	//}
}

OrderSide TBSFLAG_2_OrderSide(TBSFLAG In)
{
	switch (In)
	{
	case '1':
		return OrderSide::Buy;
	case '2':
		return OrderSide::Sell;
	default:
		return OrderSide::Buy;
	}
}

void OrderField_2_TMarketOrderFlagType(OrderField* pIn, PSTOrder pOut)
{
	// �м�ί�б���кܶ��֣�����ֻѡһ�����ײ���˫����֧�ֵ�
	// ���������嵵������ί�� 5
	switch (pIn->Type)
	{
	case OrderType::Limit:
	case OrderType::StopLimit:
	case OrderType::TrailingStopLimit:
		pOut->market_order_flag = 0;
		break;
	case OrderType::Market:
	case OrderType::MarketOnClose:
	case OrderType::Stop:
	case OrderType::TrailingStop:
		pOut->market_order_flag = '5';
	default:
		break;
	}
}

TMarketCodeType OrderField_2_TMarketCodeType(OrderField* pIn)
{
	// 1.��Լ����ʱ�͵����˶�Ӧ���г�����
	// 2.û�ж�Ӧ���г����룬ֻ�ܴӺ�Լ����ȡ
	if (strlen(pIn->ExchangeID) == 1)
		return pIn->ExchangeID[0];

	int code = atoi(pIn->InstrumentID);
	if (code>500000)
	{
		// 600000
		// 515050

		// �Ϻ�A��
		return '1';
	}
	else if (code>0)
	{
		// 000000
		// 300000
		return '2';
	}
	return '1';
}

TMarketCodeType TSecCodeType_2_TMarketCodeType(TSecCodeType* pIn)
{
	//if (strcmp())
	return '1';
}

OrderStatus TOrderStatusType_2_OrderStatus(TOrderStatusType In)
{
	switch (In)
	{
	case 0:
	case '0':
	case '1':
	case '2':
	case '3':
		return OrderStatus::New;
	case '5':
	case '6':
		return OrderStatus::Cancelled;
	case '7':
		return OrderStatus::PartiallyFilled;
	case '8':
		return OrderStatus::Filled;
	case '9':
	case 'A':
		return OrderStatus::Rejected;
	default:
		return OrderStatus::New;
	}
}

ExecType TOrderStatusType_2_ExecType(TOrderStatusType In)
{
	switch (In)
	{
	case 0:
	case '0':
	case '1':
	case '2':
	case '3':
		return ExecType::ExecNew;
	case '5':
	case '6':
		return ExecType::ExecCancelled;
	case '7':
		return ExecType::ExecTrade;
	case '8':
		return ExecType::ExecTrade;
	case '9':
	case 'A':
		return ExecType::ExecRejected;
	default:
		return ExecType::ExecNew;
	}
}

//OrderSide TThostFtdcDirectionType_2_OrderSide(TThostFtdcDirectionType In)
//{
//	if (In == THOST_FTDC_D_Sell)
//		return OrderSide::Sell;
//	return OrderSide::Buy;
//}
//
//PositionSide TThostFtdcPosiDirectionType_2_PositionSide(TThostFtdcPosiDirectionType In)
//{
//	if (In == THOST_FTDC_PD_Short)
//		return PositionSide::Short;
//	return PositionSide::Long;
//}
//
//PositionSide TradeField_2_PositionSide(TradeField* pIn)
//{
//	if (pIn->OpenClose == OpenCloseType::Open)
//	{
//		if (pIn->Side == OrderSide::Buy)
//			return PositionSide::Long;
//		return PositionSide::Short;
//	}
//	else
//	{
//		if (pIn->Side == OrderSide::Buy)
//			return PositionSide::Short;
//		return PositionSide::Long;
//	}
//}
//
//TThostFtdcOrderPriceTypeType OrderType_2_TThostFtdcOrderPriceTypeType(OrderType In)
//{
//	switch (In)
//	{
//	case Market:
//		return THOST_FTDC_OPT_AnyPrice;
//	case Stop:
//		return THOST_FTDC_OPT_AnyPrice;
//	case Limit:
//		return THOST_FTDC_OPT_LimitPrice;
//	case StopLimit:
//		return THOST_FTDC_OPT_LimitPrice;
//	case MarketOnClose:
//		return THOST_FTDC_OPT_AnyPrice;
//	case TrailingStop:
//		return THOST_FTDC_OPT_AnyPrice;
//	case TrailingStopLimit:
//		return THOST_FTDC_OPT_LimitPrice;
//	default:
//		return THOST_FTDC_OPT_LimitPrice;
//	}
//}
//
//OrderStatus CThostFtdcOrderField_2_OrderStatus(CThostFtdcOrderField* pIn)
//{
//	switch (pIn->OrderStatus)
//	{
//	case THOST_FTDC_OST_Canceled:
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
//			return OrderStatus::Rejected;
//		return OrderStatus::Cancelled;
//	case THOST_FTDC_OST_Unknown:
//		// ����ǳ�����Ҳ�п��ܳ�����һ������ι��ˣ�
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted)
//			return OrderStatus::New;
//	default:
//		if (pIn->VolumeTotal == 0)
//			return OrderStatus::Filled;
//		else if (pIn->VolumeTotal == pIn->VolumeTotalOriginal)
//			return OrderStatus::New;
//		else
//			return OrderStatus::PartiallyFilled;
//	}
//}
//
//ExecType CThostFtdcOrderField_2_ExecType(CThostFtdcOrderField* pIn)
//{
//	switch (pIn->OrderStatus)
//	{
//	case THOST_FTDC_OST_Canceled:
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
//			return ExecType::ExecRejected;
//		return ExecType::ExecCancelled;
//	case THOST_FTDC_OST_Unknown:
//		// ����ǳ�����Ҳ�п��ܳ�����һ������ι��ˣ�
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted)
//			return ExecType::ExecNew;
//	case THOST_FTDC_OST_AllTraded:
//	case THOST_FTDC_OST_PartTradedQueueing:
//		return ExecType::ExecTrade;
//	default:
//		return ExecType::ExecNew;
//	}
//}
//
//OrderStatus CThostFtdcQuoteField_2_OrderStatus(CThostFtdcQuoteField* pIn)
//{
//	switch (pIn->QuoteStatus)
//	{
//	case THOST_FTDC_OST_Canceled:
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
//			return OrderStatus::Rejected;
//		return OrderStatus::Cancelled;
//	case THOST_FTDC_OST_Unknown:
//		// ����ǳ�����Ҳ�п��ܳ�����һ������ι��ˣ�
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted)
//			return OrderStatus::New;
//	case THOST_FTDC_OST_Touched:
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
//			return OrderStatus::Rejected;
//	default:
//		// ����ط�Ҫ��
//
//		//if (pIn->VolumeTotal == 0)
//		//	return OrderStatus::Filled;
//		//else if (pIn->VolumeTotal == pIn->VolumeTotalOriginal)
//			return OrderStatus::New;
//		//else
//		//	return OrderStatus::PartiallyFilled;
//	}
//}
//
//ExecType CThostFtdcQuoteField_2_ExecType(CThostFtdcQuoteField* pIn)
//{
//	switch (pIn->QuoteStatus)
//	{
//	case THOST_FTDC_OST_Canceled:
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
//			return ExecType::ExecRejected;
//		return ExecType::ExecCancelled;
//	case THOST_FTDC_OST_Unknown:
//		// ����ǳ�����Ҳ�п��ܳ�����һ������ι��ˣ�
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted)
//			return ExecType::ExecNew;
//	case THOST_FTDC_OST_AllTraded:
//	case THOST_FTDC_OST_PartTradedQueueing:
//		return ExecType::ExecTrade;
//	case THOST_FTDC_OST_Touched:
//		if (pIn->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
//			return ExecType::ExecRejected;
//	default:
//		return ExecType::ExecNew;
//	}
//}
//
//
//
//OrderType CThostFtdcOrderField_2_OrderType(CThostFtdcOrderField* pIn)
//{
//	switch (pIn->OrderPriceType)
//	{
//	case THOST_FTDC_OPT_AnyPrice:
//		return OrderType::Market;
//	case THOST_FTDC_OPT_LimitPrice:
//		return OrderType::Limit;
//	default:
//		return OrderType::Limit;
//	}
//}
//
//TimeInForce CThostFtdcOrderField_2_TimeInForce(CThostFtdcOrderField* pIn)
//{
//	switch (pIn->TimeCondition)
//	{
//	case THOST_FTDC_TC_GFD:
//		return TimeInForce::Day;
//	case THOST_FTDC_TC_IOC:
//		switch (pIn->VolumeCondition)
//		{
//		case THOST_FTDC_VC_AV:
//			return TimeInForce::IOC;
//		case THOST_FTDC_VC_CV:
//			return TimeInForce::FOK;
//		default:
//			return TimeInForce::IOC;
//		}
//	default:
//		return TimeInForce::Day;
//	}
//}
//
//
//
//
//
//InstrumentType CThostFtdcInstrumentField_2_InstrumentType(CThostFtdcInstrumentField* pIn)
//{
//	switch (pIn->ProductClass)
//	{
//	case THOST_FTDC_PC_Futures:
//		return InstrumentType::Future;
//	case THOST_FTDC_PC_Options:
//		return InstrumentType::Option;
//	case THOST_FTDC_PC_Combination:
//		return InstrumentType::MultiLeg;
//	case THOST_FTDC_PC_EFP:
//		return InstrumentType::Future;
//	case THOST_FTDC_PC_SpotOption:
//		return InstrumentType::Option;
//	default:
//		return InstrumentType::Stock;
//	}
//}
