#include "stdafx.h"
#include "TraderApi.h"

#include "../include/QueueEnum.h"
#include "../include/QueueHeader.h"

#include "../include/ApiHeader.h"
#include "../include/ApiStruct.h"

#include "../include/toolkit.h"

#include "../QuantBox_Queue/MsgQueue.h"

#include "TypeConvert.h"

#include <cstring>
#include <assert.h>

void* __stdcall Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	// ���ڲ����ã����ü���Ƿ�Ϊ��
	CTraderApi* pApi = (CTraderApi*)pApi2;
	pApi->QueryInThread(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
	return nullptr;
}

void CTraderApi::QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int iRet = 0;
	switch (type)
	{
	case E_Init:
		iRet = _Init();
		break;
	case E_UserLoginField:
		iRet = _ReqUserLogin(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
	default:
		break;
	}

	if (0 == iRet)
	{
		//���سɹ�����ӵ��ѷ��ͳ�
		m_nSleep = 1;
	}
	else
	{
		m_msgQueue_Query->Input_Copy(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		//ʧ�ܣ���4���ݽ�����ʱ����������1s
		m_nSleep *= 4;
		m_nSleep %= 1023;
	}
	this_thread::sleep_for(chrono::milliseconds(m_nSleep));
}

CTraderApi::CTraderApi(void)
{
	m_pApi = nullptr;
	m_lRequestID = 0;
	m_nSleep = 1;

	// �Լ�ά��������Ϣ����
	m_msgQueue = new CMsgQueue();
	m_msgQueue_Query = new CMsgQueue();

	m_msgQueue_Query->Register((void*)Query,this);
	m_msgQueue_Query->StartThread();
}


CTraderApi::~CTraderApi(void)
{
	Disconnect();
}

void CTraderApi::Register(void* pCallback, void* pClass)
{
	m_pClass = pClass;
	if (m_msgQueue == nullptr)
		return;

	m_msgQueue_Query->Register((void*)Query,this);
	m_msgQueue->Register(pCallback,this);
	if (pCallback)
	{
		m_msgQueue_Query->StartThread();
		m_msgQueue->StartThread();
	}
	else
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue->StopThread();
	}
}

bool CTraderApi::IsErrorRspInfo_Output(struct DFITCErrorRtnField *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->nErrorID != 0));
	if (bRet)
	{
		ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

		pField->ErrorID = pRspInfo->nErrorID;
		strcpy(pField->ErrorMsg, pRspInfo->errorMsg);

		m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, true, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
	return bRet;
}

bool CTraderApi::IsErrorRspInfo(struct DFITCErrorRtnField *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->nErrorID != 0));

	return bRet;
}

void CTraderApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));

	m_msgQueue_Query->Input_NoCopy(RequestType::E_Init, this, nullptr, 0, 0,
		nullptr, 0, nullptr, 0, nullptr, 0);
}

int CTraderApi::_Init()
{
	m_pApi = DFITCTraderApi::CreateDFITCTraderApi();
	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Initialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Connecting, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	//��ʼ������
	int iRet = m_pApi->Init(m_ServerInfo.Address, this);
	if (0 == iRet)
	{
	}
	else
	{
		RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

		pField->ErrorID = iRet;
		strcpy(pField->ErrorMsg, "���ӳ�ʱ");

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}
	return iRet;
}

void CTraderApi::Disconnect()
{
	// �����ѯ����
	if (m_msgQueue_Query)
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue_Query->Register(nullptr,nullptr);
		m_msgQueue_Query->Clear();
		delete m_msgQueue_Query;
		m_msgQueue_Query = nullptr;
	}

	if (m_pApi)
	{
		//m_pApi->RegisterSpi(NULL);
		m_pApi->Release();
		m_pApi = NULL;

		// ȫ����ֻ�����һ��
		m_msgQueue->Clear();
		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		// ��������
		m_msgQueue->Process();
	}

	// ������Ӧ����
	if (m_msgQueue)
	{
		m_msgQueue->StopThread();
		m_msgQueue->Register(nullptr,nullptr);
		m_msgQueue->Clear();
		delete m_msgQueue;
		m_msgQueue = nullptr;
	}

	m_lRequestID = 0;

	Clear();
}

void CTraderApi::Clear()
{
	for (unordered_map<string, OrderField*>::iterator it = m_id_platform_order.begin(); it != m_id_platform_order.end(); ++it)
		delete it->second;
	m_id_platform_order.clear();

	for (unordered_map<string, DFITCOrderRtnField*>::iterator it = m_id_api_order.begin(); it != m_id_api_order.end(); ++it)
		delete it->second;
	m_id_api_order.clear();

	//for (unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.begin(); it != m_id_platform_quote.end(); ++it)
	//	delete it->second;
	//m_id_platform_quote.clear();

	for (unordered_map<string, DFITCQuoteRtnField*>::iterator it = m_id_api_quote.begin(); it != m_id_api_quote.end(); ++it)
		delete it->second;
	m_id_api_quote.clear();

	//for (unordered_map<string, PositionField*>::iterator it = m_id_platform_position.begin(); it != m_id_platform_position.end(); ++it)
	//	delete it->second;
	//m_id_platform_position.clear();
}

void CTraderApi::OnFrontConnected()
{
	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Connected, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	ReqUserLogin();
}

void CTraderApi::OnFrontDisconnected(int nReason)
{
	RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

	//����ʧ�ܷ��ص���Ϣ��ƴ�Ӷ��ɣ���Ҫ��Ϊ��ͳһ���
	pField->ErrorID = nReason;
	GetOnFrontDisconnectedMsg(nReason, pField->ErrorMsg);

	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
}

void CTraderApi::ReqUserLogin()
{
	DFITCUserLoginField* pBody = (DFITCUserLoginField*)m_msgQueue_Query->new_block(sizeof(DFITCUserLoginField));

	strncpy(pBody->accountID, m_UserInfo.UserID, sizeof(DFITCAccountIDType));
	strncpy(pBody->passwd, m_UserInfo.Password, sizeof(DFITCPasswdType));
	pBody->companyID = atoi(m_ServerInfo.BrokerID);

	m_msgQueue_Query->Input_NoCopy(RequestType::E_UserLoginField, this, nullptr, 0, 0,
		pBody, sizeof(DFITCUserLoginField), nullptr, 0, nullptr, 0);
}

int CTraderApi::_ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Logining, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	DFITCUserLoginField* pBody = (DFITCUserLoginField*)ptr1;
	pBody->lRequestID = ++m_lRequestID;
	return m_pApi->ReqUserLogin(pBody);
}

void CTraderApi::OnRspUserLogin(struct DFITCUserLoginInfoRtnField * pRspUserLogin, struct DFITCErrorRtnField * pRspInfo)
{
	RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

	if (!IsErrorRspInfo(pRspInfo)
		&& pRspUserLogin)
	{
		//strncpy(field.TradingDay, pRspUserLogin->TradingDay, sizeof(DateType));
		//strncpy(field.LoginTime, pRspUserLogin->LoginTime, sizeof(TimeType));
		sprintf(pField->SessionID, "%d", pRspUserLogin->sessionID);

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Logined, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		// ���µ�¼��Ϣ�����ܻ��õ�
		memcpy(&m_RspUserLogin, pRspUserLogin, sizeof(DFITCUserLoginInfoRtnField));
		m_nMaxOrderRef = pRspUserLogin->initLocalOrderID;
		// �Լ�����ʱID��1��ʼ�����ܴ�0��ʼ
		m_nMaxOrderRef = max(m_nMaxOrderRef, 1);
		//ReqSettlementInfoConfirm();
	}
	else
	{
		pField->ErrorID = pRspInfo->nErrorID;
		strncpy(pField->ErrorMsg, pRspInfo->errorMsg, sizeof(ErrorMsgType));

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}
}

int CTraderApi::ReqOrderInsert(
	OrderField* pOrder,
	int count,
	OrderIDType* pInOut)
{
	int OrderRef = -1;
	if (nullptr == m_pApi)
		return -1;

	DFITCInsertOrderField body = {};

	strcpy(body.accountID, m_RspUserLogin.accountID);
	// ��Լ
	strncpy(body.instrumentID, pOrder->InstrumentID, sizeof(DFITCInstrumentIDType));
	// �۸�
	body.insertPrice = pOrder->Price;
	// ����
	body.orderAmount = (DFITCAmountType)pOrder->Qty;
	// ����
	body.buySellType = OrderSide_2_DFITCBuySellTypeType(pOrder->Side);
	// ��ƽ
	body.openCloseType = OpenCloseType_2_DFITCOpenCloseTypeType(pOrder->OpenClose);
	// Ͷ��
	body.speculator = HedgeFlagType_2_DFITCSpeculatorType(pOrder->HedgeFlag);
	body.insertType = DFITC_BASIC_ORDER;
	body.orderType = OrderType_2_DFITCOrderTypeType(pOrder->Type);
	body.orderProperty = TimeInForce_2_DFITCOrderPropertyType(pOrder->TimeInForce);
	body.instrumentType = DFITC_COMM_TYPE;

	//// ��Եڶ������д�������еڶ�����������Ϊ�ǽ�����������
	//if (pOrder2)
	//{
	//	body.CombOffsetFlag[1] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
	//	body.CombHedgeFlag[1] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
	//	// ���������Ʋֻ��¹��ܣ�û��ʵ���
	//	body.IsSwapOrder = (body.CombOffsetFlag[0] != body.CombOffsetFlag[1]);
	//}

	//�۸�
	//body.OrderPriceType = OrderType_2_TThostFtdcOrderPriceTypeType(pOrder1->Type);


	int nRet = 0;
	{
		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
		lock_guard<mutex> cl(m_csOrderRef);

		if (OrderRef < 0)
		{
			nRet = m_nMaxOrderRef;
			++m_nMaxOrderRef;
		}
		else
		{
			nRet = OrderRef;
		}
		body.localOrderID = nRet;

		//�����浽���У�����ֱ�ӷ���
		int n = m_pApi->ReqInsertOrder(&body);
		if (n < 0)
		{
			nRet = n;
			sprintf(m_orderInsert_Id, "%d", nRet);
		}
		else
		{
			// ���ڸ���������ҵ�ԭ���������ڽ�����Ӧ��֪ͨ
			OrderIDType orderId = { 0 };
			sprintf(m_orderInsert_Id, "%d:%d", m_RspUserLogin.sessionID, nRet);

			OrderField* pField = new OrderField();
			memcpy(pField, pOrder, sizeof(OrderField));
			strcpy(pField->ID, m_orderInsert_Id);
			m_id_platform_order.insert(pair<string, OrderField*>(m_orderInsert_Id, pField));
		}
		strncpy((char*)pInOut, m_orderInsert_Id, sizeof(OrderIDType));
	}

	return nRet;
}

void CTraderApi::OnRspInsertOrder(struct DFITCOrderRspDataRtnField * pOrderRtn, struct DFITCErrorRtnField * pErrorInfo)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d", m_RspUserLogin.sessionID, pOrderRtn->localOrderID);

	unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	if (it == m_id_platform_order.end())
	{
		// û�ҵ�����Ӧ�������ʾ������
		//assert(false);
	}
	else
	{
		// �ҵ��ˣ�Ҫ����״̬
		// ��ʹ���ϴε�״̬
		OrderField* pField = it->second;
		pField->ExecType = ExecType::ExecRejected;
		pField->Status = OrderStatus::Rejected;
		pField->ErrorID = pErrorInfo->nErrorID;
		strncpy(pField->Text, pErrorInfo->errorMsg, sizeof(ErrorMsgType));
		m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}
//
//void CTraderApi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
//{
//	OrderIDType orderId = { 0 };
//	sprintf(orderId, "%d:%d:%s", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, pInputOrder->OrderRef);
//
//	hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
//	if (it == m_id_platform_order.end())
//	{
//		// û�ҵ�����Ӧ�������ʾ������
//		assert(false);
//	}
//	else
//	{
//		// �ҵ��ˣ�Ҫ����״̬
//		// ��ʹ���ϴε�״̬
//		OrderField* pField = it->second;
//		pField->ExecType = ExecType::ExecRejected;
//		pField->Status = OrderStatus::Rejected;
//		pField->ErrorID = pRspInfo->ErrorID;
//		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
//		m_msgQueue->Input(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
//	}
//}

//int CTraderApi::ReqParkedOrderInsert(
//	int OrderRef,
//	OrderField* pOrder1,
//	OrderField* pOrder2)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_ParkedOrderField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcParkedOrderField& body = pRequest->ParkedOrderField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	body.MinVolume = 1;
//	body.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
//	body.IsAutoSuspend = 0;
//	body.UserForceClose = 0;
//	body.IsSwapOrder = 0;
//
//	//��Լ
//	strncpy(body.InstrumentID, pOrder1->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	//����
//	body.Direction = OrderSide_2_TThostFtdcDirectionType(pOrder1->Side);
//	//��ƽ
//	body.CombOffsetFlag[0] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
//	//Ͷ��
//	body.CombHedgeFlag[0] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
//	//����
//	body.VolumeTotalOriginal = (int)pOrder1->Qty;
//
//	// ���������������õ�һ�������ļ۸񣬻��������������ļ۸���أ�
//	body.LimitPrice = pOrder1->Price;
//	body.StopPrice = pOrder1->StopPx;
//
//	// ��Եڶ������д�������еڶ�����������Ϊ�ǽ�����������
//	if (pOrder2)
//	{
//		body.CombOffsetFlag[1] = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrder1->OpenClose);
//		body.CombHedgeFlag[1] = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrder1->HedgeFlag);
//		// ���������Ʋֻ��¹��ܣ�û��ʵ���
//		body.IsSwapOrder = (body.CombOffsetFlag[0] != body.CombOffsetFlag[1]);
//	}
//
//	//�۸�
//	//body.OrderPriceType = OrderType_2_TThostFtdcOrderPriceTypeType(pOrder1->Type);
//
//	// �м����޼�
//	switch (pOrder1->Type)
//	{
//	case Market:
//	case Stop:
//	case MarketOnClose:
//	case TrailingStop:
//		body.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
//		body.TimeCondition = THOST_FTDC_TC_IOC;
//		break;
//	case Limit:
//	case StopLimit:
//	case TrailingStopLimit:
//	default:
//		body.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
//		body.TimeCondition = THOST_FTDC_TC_GFD;
//		break;
//	}
//
//	// IOC��FOK
//	switch (pOrder1->TimeInForce)
//	{
//	case IOC:
//		body.TimeCondition = THOST_FTDC_TC_IOC;
//		body.VolumeCondition = THOST_FTDC_VC_AV;
//		break;
//	case FOK:
//		body.TimeCondition = THOST_FTDC_TC_IOC;
//		body.VolumeCondition = THOST_FTDC_VC_CV;
//		//body.MinVolume = body.VolumeTotalOriginal; // ����ط��������
//		break;
//	default:
//		body.VolumeCondition = THOST_FTDC_VC_AV;
//		break;
//	}
//
//	// ������
//	switch (pOrder1->Type)
//	{
//	case Stop:
//	case TrailingStop:
//	case StopLimit:
//	case TrailingStopLimit:
//		// ������û�в��ԣ�������
//		body.ContingentCondition = THOST_FTDC_CC_Immediately;
//		break;
//	default:
//		body.ContingentCondition = THOST_FTDC_CC_Immediately;
//		break;
//	}
//
//	int nRet = 0;
//	{
//		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
//		lock_guard<mutex> cl(m_csOrderRef);
//
//		if (OrderRef < 0)
//		{
//			nRet = m_nMaxOrderRef;
//			++m_nMaxOrderRef;
//		}
//		else
//		{
//			nRet = OrderRef;
//		}
//		sprintf(body.OrderRef, "%d", nRet);
//
//		//�����浽���У�����ֱ�ӷ���
//		int n = m_pApi->ReqParkedOrderInsert(&pRequest->ParkedOrderField, ++m_lRequestID);
//		if (n < 0)
//		{
//			nRet = n;
//		}
//		else
//		{
//			// ���ڸ���������ҵ�ԭ���������ڽ�����Ӧ��֪ͨ
//			OrderIDType orderId = { 0 };
//			sprintf(orderId, "%d:%d:%d", m_RspUserLogin.FrontID, m_RspUserLogin.SessionID, nRet);
//
//			OrderField* pField = new OrderField();
//			memcpy(pField, pOrder1, sizeof(OrderField));
//			m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));
//		}
//	}
//	delete pRequest;//�����ֱ��ɾ��
//
//	return nRet;
//}

void CTraderApi::OnRtnMatchedInfo(struct DFITCMatchRtnField * pRtnMatchData)
{
	OnTrade(pRtnMatchData);
}

//int CTraderApi::ReqOrderAction(const string& szId)
//{
//	hash_map<string, CThostFtdcOrderField*>::iterator it = m_id_api_order.find(szId);
//	if (it == m_id_api_order.end())
//	{
//		// <error id="ORDER_NOT_FOUND" value="25" prompt="CTP:�����Ҳ�����Ӧ����"/>
//		//ErrorField field = { 0 };
//		//field.ErrorID = 25;
//		//sprintf(field.ErrorMsg, "ORDER_NOT_FOUND");
//
//		////TODO:Ӧ��ͨ�������ر�֪ͨ�����Ҳ���
//
//		//m_msgQueue->Input(ResponeType::OnRtnError, m_msgQueue, this, 0, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
//		return -100;
//	}
//	else
//	{
//		// �ҵ��˶���
//		return ReqOrderAction(it->second);
//	}
//}
//
//int CTraderApi::ReqOrderAction(CThostFtdcOrderField *pOrder)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_InputOrderActionField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcInputOrderActionField& body = pRequest->InputOrderActionField;
//
//	///���͹�˾����
//	strncpy(body.BrokerID, pOrder->BrokerID, sizeof(TThostFtdcBrokerIDType));
//	///Ͷ���ߴ���
//	strncpy(body.InvestorID, pOrder->InvestorID, sizeof(TThostFtdcInvestorIDType));
//	///��������
//	strncpy(body.OrderRef, pOrder->OrderRef, sizeof(TThostFtdcOrderRefType));
//	///ǰ�ñ��
//	body.FrontID = pOrder->FrontID;
//	///�Ự���
//	body.SessionID = pOrder->SessionID;
//	///����������
//	strncpy(body.ExchangeID, pOrder->ExchangeID, sizeof(TThostFtdcExchangeIDType));
//	///�������
//	strncpy(body.OrderSysID, pOrder->OrderSysID, sizeof(TThostFtdcOrderSysIDType));
//	///������־
//	body.ActionFlag = THOST_FTDC_AF_Delete;
//	///��Լ����
//	strncpy(body.InstrumentID, pOrder->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//
//	int nRet = m_pApi->ReqOrderAction(&pRequest->InputOrderActionField, ++m_lRequestID);
//	delete pRequest;
//	return nRet;
//}

void CTraderApi::OnRspCancelOrder(struct DFITCOrderRspDataRtnField *pOrderCanceledRtn, struct DFITCErrorRtnField *pErrorInfo)
{
	//OrderIDType orderId = { 0 };
	//sprintf(orderId, "%d:%d:%s", , pInputOrderAction->SessionID, pInputOrderAction->OrderRef);

	//hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
	//if (it == m_id_platform_order.end())
	//{
	//	// û�ҵ�����Ӧ�������ʾ������
	//	assert(false);
	//}
	//else
	//{
	//	// �ҵ��ˣ�Ҫ����״̬
	//	// ��ʹ���ϴε�״̬
	//	OrderField* pField = it->second;
	//	strcpy(pField->ID, orderId);
	//	pField->ExecType = ExecType::ExecCancelReject;
	//	pField->ErrorID = pRspInfo->ErrorID;
	//	strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
	//	m_msgQueue->Input(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	//}
}

//void CTraderApi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
//{
//	OrderIDType orderId = { 0 };
//	sprintf(orderId, "%d:%d:%s", pOrderAction->FrontID, pOrderAction->SessionID, pOrderAction->OrderRef);
//
//	hash_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
//	if (it == m_id_platform_order.end())
//	{
//		// û�ҵ�����Ӧ�������ʾ������
//		assert(false);
//	}
//	else
//	{
//		// �ҵ��ˣ�Ҫ����״̬
//		// ��ʹ���ϴε�״̬
//		OrderField* pField = it->second;
//		strcpy(pField->ID, orderId);
//		pField->ExecType = ExecType::ExecCancelReject;
//		pField->ErrorID = pRspInfo->ErrorID;
//		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(TThostFtdcErrorMsgType));
//		m_msgQueue->Input(ResponeType::OnRtnOrder, m_msgQueue, this, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
//	}
//}

void CTraderApi::OnRtnOrder(struct DFITCOrderRtnField * pRtnOrderData)
{
	OnOrder(pRtnOrderData);
}

//int CTraderApi::ReqQuoteInsert(
//	int QuoteRef,
//	OrderField* pOrderAsk,
//	OrderField* pOrderBid)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_InputQuoteField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcInputQuoteField& body = pRequest->InputQuoteField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	//��Լ,Ŀǰֻ�Ӷ���1��ȡ
//	strncpy(body.InstrumentID, pOrderAsk->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//	//��ƽ
//	body.AskOffsetFlag = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrderAsk->OpenClose);
//	body.BidOffsetFlag = OpenCloseType_2_TThostFtdcOffsetFlagType(pOrderBid->OpenClose);
//	//Ͷ��
//	body.AskHedgeFlag = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrderAsk->HedgeFlag);
//	body.BidHedgeFlag = HedgeFlagType_2_TThostFtdcHedgeFlagType(pOrderBid->HedgeFlag);
//
//	//�۸�
//	body.AskPrice = pOrderAsk->Price;
//	body.BidPrice = pOrderBid->Price;
//
//	//����
//	body.AskVolume = (int)pOrderAsk->Qty;
//	body.BidVolume = (int)pOrderBid->Qty;
//
//	int nRet = 0;
//	{
//		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
//		lock_guard<mutex> cl(m_csOrderRef);
//
//		if (QuoteRef < 0)
//		{
//			nRet = m_nMaxOrderRef;
//			sprintf(body.QuoteRef, "%d", m_nMaxOrderRef);
//			sprintf(body.AskOrderRef, "%d", m_nMaxOrderRef);
//			sprintf(body.BidOrderRef, "%d", ++m_nMaxOrderRef);
//			++m_nMaxOrderRef;
//		}
//		else
//		{
//			nRet = QuoteRef;
//			sprintf(body.QuoteRef, "%d", QuoteRef);
//			sprintf(body.AskOrderRef, "%d", QuoteRef);
//			sprintf(body.BidOrderRef, "%d", ++QuoteRef);
//			++QuoteRef;
//		}
//
//		//�����浽���У�����ֱ�ӷ���
//		int n = m_pApi->ReqQuoteInsert(&pRequest->InputQuoteField, ++m_lRequestID);
//		if (n < 0)
//		{
//			nRet = n;
//		}
//	}
//	delete pRequest;//�����ֱ��ɾ��
//
//	return nRet;
//}
//
//void CTraderApi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnRspQuoteInsert(this, pInputQuote, pRspInfo, nRequestID, bIsLast);
//}
//
//void CTraderApi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnErrRtnQuoteInsert(this, pInputQuote, pRspInfo);
//}
//
//void CTraderApi::OnRtnQuote(CThostFtdcQuoteField *pQuote)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnRtnQuote(this, pQuote);
//}
//
//int CTraderApi::ReqQuoteAction(const string& szId)
//{
//	hash_map<string, CThostFtdcQuoteField*>::iterator it = m_id_api_quote.find(szId);
//	if (it == m_id_api_quote.end())
//	{
//		// <error id="QUOTE_NOT_FOUND" value="86" prompt="CTP:���۳����Ҳ�����Ӧ����"/>
//		ErrorField field = { 0 };
//		field.ErrorID = 86;
//		sprintf(field.ErrorMsg, "QUOTE_NOT_FOUND");
//
//		m_msgQueue->Input(ResponeType::OnRtnError, m_msgQueue, this, 0, 0, &field, sizeof(ErrorField), nullptr, 0, nullptr, 0);
//	}
//	else
//	{
//		// �ҵ��˶���
//		ReqQuoteAction(it->second);
//	}
//	return 0;
//}
//
//int CTraderApi::ReqQuoteAction(CThostFtdcQuoteField *pQuote)
//{
//	if (nullptr == m_pApi)
//		return 0;
//
//	SRequest* pRequest = MakeRequestBuf(E_InputQuoteActionField);
//	if (nullptr == pRequest)
//		return 0;
//
//	CThostFtdcInputQuoteActionField& body = pRequest->InputQuoteActionField;
//
//	///���͹�˾����
//	strncpy(body.BrokerID, pQuote->BrokerID, sizeof(TThostFtdcBrokerIDType));
//	///Ͷ���ߴ���
//	strncpy(body.InvestorID, pQuote->InvestorID, sizeof(TThostFtdcInvestorIDType));
//	///��������
//	strncpy(body.QuoteRef, pQuote->QuoteRef, sizeof(TThostFtdcOrderRefType));
//	///ǰ�ñ��
//	body.FrontID = pQuote->FrontID;
//	///�Ự���
//	body.SessionID = pQuote->SessionID;
//	///����������
//	strncpy(body.ExchangeID, pQuote->ExchangeID, sizeof(TThostFtdcExchangeIDType));
//	///�������
//	strncpy(body.QuoteSysID, pQuote->QuoteSysID, sizeof(TThostFtdcOrderSysIDType));
//	///������־
//	body.ActionFlag = THOST_FTDC_AF_Delete;
//	///��Լ����
//	strncpy(body.InstrumentID, pQuote->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
//
//	int nRet = m_pApi->ReqQuoteAction(&pRequest->InputQuoteActionField, ++m_lRequestID);
//	delete pRequest;
//	return nRet;
//}
//
//void CTraderApi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnRspQuoteAction(this, pInputQuoteAction, pRspInfo, nRequestID, bIsLast);
//}
//
//void CTraderApi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo)
//{
//	//if (m_msgQueue)
//	//	m_msgQueue->Input_OnErrRtnQuoteAction(this, pQuoteAction, pRspInfo);
//}

void CTraderApi::ReqQryCustomerCapital()
{
	DFITCCapitalField* pBody = (DFITCCapitalField*)m_msgQueue_Query->new_block(sizeof(DFITCCapitalField));


	strcpy(pBody->accountID, m_RspUserLogin.accountID);

	m_msgQueue_Query->Input_NoCopy(RequestType::E_CapitalField, this, nullptr, 0, 0,
		pBody, sizeof(DFITCCapitalField), nullptr, 0, nullptr, 0);
}

int CTraderApi::_ReqQryCustomerCapital(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	DFITCCapitalField* pBody = (DFITCCapitalField*)ptr1;
	pBody->lRequestID = ++m_lRequestID;
	return m_pApi->ReqQryCustomerCapital(pBody);
}

void CTraderApi::OnRspCustomerCapital(struct DFITCCapitalInfoRtnField * pCapitalInfoRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
{
	if (!IsErrorRspInfo(pErrorInfo))
	{
		if (pCapitalInfoRtn)
		{
			AccountField* pField = (AccountField*)m_msgQueue->new_block(sizeof(AccountField));

			strcpy(pField->Account, pCapitalInfoRtn->accountID);
			pField->PreBalance = pCapitalInfoRtn->preEquity;
			pField->CurrMargin = pCapitalInfoRtn->margin;
			pField->Commission = pCapitalInfoRtn->fee;
			pField->CloseProfit = pCapitalInfoRtn->closeProfitLoss;
			pField->PositionProfit = pCapitalInfoRtn->positionProfitLoss;
			pField->Balance = pCapitalInfoRtn->todayEquity;
			pField->Available = pCapitalInfoRtn->available;

			m_msgQueue->Input_NoCopy(ResponeType::OnRspQryTradingAccount, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(AccountField), nullptr, 0, nullptr, 0);
		}
		else
		{
			m_msgQueue->Input_NoCopy(ResponeType::OnRspQryTradingAccount, m_msgQueue, m_pClass, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}
}

//void CTraderApi::ReqQryPosition(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_PositionField);
//	if (nullptr == pRequest)
//		return;
//
//	DFITCPositionField* body = (DFITCPositionField*)pRequest->pBuf;
//
//	strncpy(body->accountID, m_RspUserLogin.accountID, sizeof(DFITCAccountIDType));
//	strncpy(body->instrumentID, szInstrumentId.c_str(), sizeof(DFITCInstrumentIDType));
//	//body->instrumentType;
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryPosition(struct DFITCPositionInfoRtnField * pPositionInfoRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInvestorPosition(this,pInvestorPosition,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(pErrorInfo->requestID);
//}
//
//void CTraderApi::ReqQryInvestorPositionDetail(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryInvestorPositionDetailField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryInvestorPositionDetailField& body = pRequest->QryInvestorPositionDetailField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInvestorPositionDetail(this,pInvestorPositionDetail,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

void CTraderApi::ReqQryExchangeInstrument(const string& szExchangeId, DFITCInstrumentTypeType instrumentType)
{
	DFITCExchangeInstrumentField body = {};

	//strcpy(body->accountID, m_RspUserLogin.accountID);
	//strncpy(body->exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
	//body->instrumentType = instrumentType;

	//AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryExchangeInstrument(struct DFITCExchangeInstrumentRtnField * pInstrumentData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
{
	if (!IsErrorRspInfo(pErrorInfo))
	{
		if (pInstrumentData)
		{
			InstrumentField* pField = (InstrumentField*)m_msgQueue->new_block(sizeof(InstrumentField));

			strncpy(pField->InstrumentID, pInstrumentData->instrumentID, sizeof(InstrumentIDType));
			strncpy(pField->ExchangeID, pInstrumentData->exchangeID, sizeof(ExchangeIDType));

			strncpy(pField->Symbol, pInstrumentData->instrumentID, sizeof(SymbolType));

			strncpy(pField->InstrumentName, pInstrumentData->VarietyName, sizeof(InstrumentNameType));
			pField->Type = DFITCInstrumentTypeType_2_InstrumentType(pInstrumentData->instrumentType);
			pField->VolumeMultiple = (VolumeMultipleType)pInstrumentData->contractMultiplier;
			pField->PriceTick = pInstrumentData->minPriceFluctuation;
			pField->ExpireDate = GetDate(pInstrumentData->instrumentMaturity);
			//pField->OptionsType = TThostFtdcOptionsTypeType_2_PutCall(pInstrument->OptionsType);

			m_msgQueue->Input_NoCopy(ResponeType::OnRspQryInstrument, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(InstrumentField), nullptr, 0, nullptr, 0);
		}
		else
		{
			m_msgQueue->Input_NoCopy(ResponeType::OnRspQryInstrument, m_msgQueue, m_pClass, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}
}


void CTraderApi::ReqQryArbitrageInstrument(const string& szExchangeId)
{
	DFITCAbiInstrumentField body = {};

	strcpy(body.accountID, m_RspUserLogin.accountID);
	strncpy(body.exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
}

void CTraderApi::OnRspArbitrageInstrument(struct DFITCAbiInstrumentRtnField * pAbiInstrumentData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast)
{
	if (!IsErrorRspInfo(pErrorInfo))
	{
		if (pAbiInstrumentData)
		{
			InstrumentField* pField = (InstrumentField*)m_msgQueue->new_block(sizeof(InstrumentField));

			strncpy(pField->InstrumentID, pAbiInstrumentData->InstrumentID, sizeof(InstrumentIDType));
			strncpy(pField->ExchangeID, pAbiInstrumentData->exchangeID, sizeof(ExchangeIDType));

			strncpy(pField->Symbol, pAbiInstrumentData->InstrumentID, sizeof(SymbolType));

			strncpy(pField->InstrumentName, pAbiInstrumentData->instrumentName, sizeof(InstrumentNameType));
			/*pField->Type = DFITCInstrumentTypeType_2_InstrumentType(pInstrumentData->instrumentType);
			pField->VolumeMultiple = pInstrumentData->contractMultiplier;
			pField->PriceTick = pInstrumentData->minPriceFluctuation;
			strncpy(pField->ExpireDate, pInstrumentData->instrumentMaturity, sizeof(DFITCInstrumentMaturityType));*/
			//pField->OptionsType = TThostFtdcOptionsTypeType_2_PutCall(pInstrument->OptionsType);

			m_msgQueue->Input_NoCopy(ResponeType::OnRspQryInstrument, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(InstrumentField), nullptr, 0, nullptr, 0);
		}
		else
		{
			m_msgQueue->Input_NoCopy(ResponeType::OnRspQryInstrument, m_msgQueue, m_pClass, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}
}

//void CTraderApi::ReqQryInstrumentCommissionRate(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentCommissionRateField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryInstrumentCommissionRateField& body = pRequest->QryInstrumentCommissionRateField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInstrumentCommissionRate(this,pInstrumentCommissionRate,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQryInstrumentMarginRate(const string& szInstrumentId, TThostFtdcHedgeFlagType HedgeFlag)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryInstrumentMarginRateField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryInstrumentMarginRateField& body = pRequest->QryInstrumentMarginRateField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//	body.HedgeFlag = HedgeFlag;
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryInstrumentMarginRate(this,pInstrumentMarginRate,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQryDepthMarketData(const string& szInstrumentId)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryDepthMarketDataField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryDepthMarketDataField& body = pRequest->QryDepthMarketDataField;
//
//	strncpy(body.InstrumentID, szInstrumentId.c_str(), sizeof(TThostFtdcInstrumentIDType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRspQryDepthMarketData(this,pDepthMarketData,pRspInfo,nRequestID,bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQrySettlementInfo(const string& szTradingDay)
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QrySettlementInfoField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQrySettlementInfoField& body = pRequest->QrySettlementInfoField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//	strncpy(body.TradingDay, szTradingDay.c_str(), sizeof(TThostFtdcDateType));
//
//	AddToSendQueue(pRequest);
//}
//
//void CTraderApi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
//	{
//		if (pSettlementInfo)
//		{
//			SettlementInfoField field = { 0 };
//			strncpy(field.TradingDay, pSettlementInfo->TradingDay, sizeof(TThostFtdcDateType));
//			strncpy(field.Content, pSettlementInfo->Content, sizeof(TThostFtdcContentType));
//
//			m_msgQueue->Input(ResponeType::OnRspQrySettlementInfo, m_msgQueue, this, bIsLast, 0, &field, sizeof(SettlementInfoField), nullptr, 0, nullptr, 0);
//		}
//		else
//		{
//			m_msgQueue->Input(ResponeType::OnRspQrySettlementInfo, m_msgQueue, this, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
//		}
//	}
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

//void CTraderApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	IsErrorRspInfo(pRspInfo, nRequestID, bIsLast);
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}
//
//void CTraderApi::ReqQryOrder()
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryOrderField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryOrderField& body = pRequest->QryOrderField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	AddToSendQueue(pRequest);
//}

void CTraderApi::OnOrder(DFITCOrderRtnField *pOrder)
{
	if (nullptr == pOrder)
		return;

	OrderIDType orderId = { 0 };
	sprintf(orderId, "%ld:%ld", pOrder->sessionID, pOrder->localOrderID);
	OrderIDType orderSydId = { 0 };

	{
		// ����ԭʼ������Ϣ�����ڳ���

		unordered_map<string, DFITCOrderRtnField*>::iterator it = m_id_api_order.find(orderId);
		if (it == m_id_api_order.end())
		{
			// �Ҳ����˶�������ʾ���µ�
			DFITCOrderRtnField* pField = new DFITCOrderRtnField();
			memcpy(pField, pOrder, sizeof(DFITCOrderRtnField));
			m_id_api_order.insert(pair<string, DFITCOrderRtnField*>(orderId, pField));
		}
		else
		{
			// �ҵ��˶���
			// ��Ҫ�ٸ��Ʊ������һ�ε�״̬������ֻҪ��һ�ε����ڳ������ɣ����£��������ñȽ�
			DFITCOrderRtnField* pField = it->second;
			memcpy(pField, pOrder, sizeof(DFITCOrderRtnField));
		}

		// ����SysID���ڶ���ɽ��ر��붩��
		sprintf(orderSydId, "%s:%s", pOrder->exchangeID, pOrder->OrderSysID);
		m_sysId_orderId.insert(pair<string, string>(orderSydId, orderId));
	}

	{
		// ��API�Ķ���ת�����Լ��Ľṹ��

		OrderField* pField = nullptr;
		unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
		if (it == m_id_platform_order.end())
		{
			// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
			pField = (OrderField*)m_msgQueue->new_block(sizeof(OrderField));

			strcpy(pField->ID, orderId);
			strcpy(pField->InstrumentID, pOrder->instrumentID);
			strcpy(pField->ExchangeID, pOrder->exchangeID);
			pField->HedgeFlag = DFITCSpeculatorType_2_HedgeFlagType(pOrder->speculator);
			pField->Side = DFITCBuySellTypeType_2_OrderSide(pOrder->buySellType);
			pField->Price = pOrder->insertPrice;
			pField->StopPx = 0;
			//strncpy(pField->Text, pOrder->, sizeof(TThostFtdcErrorMsgType));
			pField->OpenClose = DFITCOpenCloseTypeType_2_OpenCloseType(pOrder->openCloseType);
			pField->Status = DFITCOrderRtnField_2_OrderStatus(pOrder);
			pField->Qty = pOrder->orderAmount;
			pField->Type = DFITCOrderRtnField_2_OrderType(pOrder);
			pField->TimeInForce = DFITCOrderRtnField_2_TimeInForce(pOrder);
			pField->ExecType = ExecType::ExecNew;
			strcpy(pField->OrderID, pOrder->OrderSysID);


			// ��ӵ�map�У������������ߵĶ�ȡ������ʧ��ʱ����֪ͨ��
			m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));
		}
		else
		{
			pField = it->second;
			strcpy(pField->ID, orderId);
			//pField->LeavesQty = pOrder->;
			pField->Price = pOrder->insertPrice;
			pField->Status = DFITCOrderRtnField_2_OrderStatus(pOrder);
			pField->ExecType = DFITCOrderRtnField_2_ExecType(pOrder);
			strcpy(pField->OrderID, pOrder->OrderSysID);
			//strncpy(pField->Text, pOrder->StatusMsg, sizeof(TThostFtdcErrorMsgType));
		}

		m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
	}
}

//void CTraderApi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
//	{
//		OnOrder(pOrder);
//	}
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

//void CTraderApi::ReqQryTrade()
//{
//	if (nullptr == m_pApi)
//		return;
//
//	SRequest* pRequest = MakeRequestBuf(E_QryTradeField);
//	if (nullptr == pRequest)
//		return;
//
//	CThostFtdcQryTradeField& body = pRequest->QryTradeField;
//
//	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TThostFtdcBrokerIDType));
//	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TThostFtdcInvestorIDType));
//
//	AddToSendQueue(pRequest);
//}

void CTraderApi::OnTrade(DFITCMatchRtnField *pTrade)
{
	if (nullptr == pTrade)
		return;

	TradeField* pField = (TradeField*)m_msgQueue->new_block(sizeof(TradeField));

	strcpy(pField->InstrumentID, pTrade->instrumentID);
	strcpy(pField->ExchangeID, pTrade->exchangeID);
	pField->Side = DFITCBuySellTypeType_2_OrderSide(pTrade->buySellType);
	pField->Qty = pTrade->matchedAmount;
	//pField->Price = pTrade->Price;
	//pField->OpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pTrade->OffsetFlag);
	//pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);
	//pField->Commission = 0;//TODO�������Ժ�Ҫ�������
	//strncpy(pField->Time, pTrade->TradeTime, sizeof(TThostFtdcTimeType));
	//strcpy(pField->TradeID, pTrade->TradeID);

	//OrderIDType orderSysId = { 0 };
	//sprintf(orderSysId, "%s:%s", pTrade->ExchangeID, pTrade->OrderSysID);
	//hash_map<string, string>::iterator it = m_sysId_orderId.find(orderSysId);
	//if (it == m_sysId_orderId.end())
	//{
	//	// �˳ɽ��Ҳ�����Ӧ�ı���
	//	assert(false);
	//}
	//else
	//{
	//	// �ҵ���Ӧ�ı���
	//	strcpy(pField->ID, it->second.c_str());

	//	m_msgQueue->Input(ResponeType::OnRtnTrade, m_msgQueue, this, 0, 0, pField, sizeof(TradeField), nullptr, 0, nullptr, 0);

	//	hash_map<string, OrderField*>::iterator it2 = m_id_platform_order.find(it->second);
	//	if (it2 == m_id_platform_order.end())
	//	{
	//		// �˳ɽ��Ҳ�����Ӧ�ı���
	//		assert(false);
	//	}
	//	else
	//	{
	//		// ���¶�����״̬
	//		// �Ƿ�Ҫ֪ͨ�ӿ�
	//	}
	//}
}

//void CTraderApi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	if (!IsErrorRspInfo(pRspInfo, nRequestID, bIsLast))
//	{
//		OnTrade(pTrade);
//	}
//
//	if (bIsLast)
//		ReleaseRequestMapBuf(nRequestID);
//}

//void CTraderApi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
//{
//	//if(m_msgQueue)
//	//	m_msgQueue->Input_OnRtnInstrumentStatus(this,pInstrumentStatus);
//}

void CTraderApi::ReqQuoteSubscribe(const string& szExchangeId, DFITCInstrumentTypeType instrumentType)
{
	DFITCQuoteSubscribeField body = {};

	strcpy(body.accountID, m_RspUserLogin.accountID);
	strncpy(body.exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
	body.instrumentType = instrumentType;

	//AddToSendQueue(pRequest);
}

void CTraderApi::ReqQuoteUnSubscribe(const string& szExchangeId, DFITCInstrumentTypeType instrumentType)
{
	DFITCQuoteUnSubscribeField body = {};

	strcpy(body.accountID, m_RspUserLogin.accountID);
	strncpy(body.exchangeID, szExchangeId.c_str(), sizeof(DFITCExchangeIDType));
	body.instrumentType = instrumentType;
}


void CTraderApi::OnRspQuoteSubscribe(struct DFITCQuoteSubscribeRspField * pRspQuoteSubscribeData)
{

}

void CTraderApi::OnRtnQuoteSubscribe(struct DFITCQuoteSubscribeRtnField * pRtnQuoteSubscribeData)
{
	QuoteRequestField* pField = (QuoteRequestField*)m_msgQueue->new_block(sizeof(QuoteRequestField));

	pField->TradingDay = GetDate(pRtnQuoteSubscribeData->tradingDate);
	pField->QuoteTime = GetDate(pRtnQuoteSubscribeData->quoteTime);
	strcpy(pField->Symbol, pRtnQuoteSubscribeData->instrumentID);
	strcpy(pField->InstrumentID, pRtnQuoteSubscribeData->instrumentID);
	strcpy(pField->ExchangeID, pRtnQuoteSubscribeData->exchangeID);
	strcpy(pField->QuoteID, pRtnQuoteSubscribeData->quoteID);

	m_msgQueue->Input_NoCopy(ResponeType::OnRtnQuoteRequest, m_msgQueue, m_pClass, 0, 0, pField, sizeof(QuoteRequestField), nullptr, 0, nullptr, 0);
}
