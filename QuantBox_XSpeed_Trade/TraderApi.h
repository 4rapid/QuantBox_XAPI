#pragma once

#include "../include/XSpeed/DFITCTraderApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/XSpeed/win64/DFITCTraderApi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/XSpeed/win32/DFITCTraderApi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <list>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <unordered_map>

using namespace std;
using namespace DFITCXSPEEDAPI;

class CMsgQueue;

class CTraderApi :
	public DFITCTraderSpi
{
	//�������ݰ�����
	enum RequestType
	{
		E_AbiInstrumentField,
		E_CancelOrderField,
		E_CapitalField,
		E_ExchangeInstrumentField,
		E_Init,
		E_InsertOrderField,
		E_MatchField,
		E_OrderField,
		E_PositionField,
		E_PositionDetailField,
		E_SpecificInstrumentField,
		E_UserLoginField,
		E_QuoteSubscribeField,
		E_QuoteUnSubscribeField,
		E_QuoteInsertField,
		E_QuoteCancelField,
	};

public:
	CTraderApi(void);
	virtual ~CTraderApi(void);

	void Register(void* pCallback, void* pClass);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	int ReqOrderInsert(
		OrderField* pOrder,
		int count,
		OrderIDType* pInOut);

	int ReqOrderAction(OrderIDType* szIds, int count, OrderIDType* pOutput);
	void ReqCancelOrder(
		const string& szInstrumentId,
		DFITCLocalOrderIDType localOrderID,
		DFITCSPDOrderIDType spdOrderID);

	int ReqQuoteInsert(
		int QuoteRef,
		OrderField* pOrderAsk,
		OrderField* pOrderBid);

	int ReqQuoteAction(const string& szId);
	void ReqQuoteCancelOrder(
		const string& szInstrumentId,
		DFITCLocalOrderIDType localOrderID,
		DFITCSPDOrderIDType spdOrderID);


	void ReqQryMatchInfo(DFITCInstrumentTypeType instrumentType);
	void ReqQryOrderInfo(DFITCInstrumentTypeType instrumentType);
	void ReqQryCustomerCapital();
	void ReqQryPosition(const string& szInstrumentId);
	//void ReqQryPositionDetail(const string& szInstrumentId);
	void ReqQryExchangeInstrument(const string& szExchangeId, DFITCInstrumentTypeType instrumentType);
	void ReqQryArbitrageInstrument(const string& szExchangeId);
	void ReqQrySpecifyInstrument(const string& szInstrumentId, const string& szExchangeId, DFITCInstrumentTypeType instrumentType);
	
	void ReqQuoteSubscribe(const string& szExchangeId, DFITCInstrumentTypeType instrumentType);
	void ReqQuoteUnSubscribe(const string& szExchangeId, DFITCInstrumentTypeType instrumentType);

private:
	friend void* __stdcall Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	void QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	void Clear();

	int _Init();

	void ReqUserLogin();
	int _ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	int _ReqQryCustomerCapital(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	void OnOrder(DFITCOrderRtnField *pOrder);
	void OnTrade(DFITCMatchRtnField *pTrade);

	//int ReqInit();

	//void ReqAuthenticate();
	//void ReqUserLogin();
	//void ReqSettlementInfoConfirm();

	//����Ƿ����
	bool IsErrorRspInfo_Output(struct DFITCErrorRtnField *pRspInfo);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(struct DFITCErrorRtnField *pRspInfo); //�������Ϣ

	//����
	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);

	//��֤
	virtual void OnRspUserLogin(struct DFITCUserLoginInfoRtnField *pRspUserLogin, struct DFITCErrorRtnField *pRspInfo);
	virtual void OnRspUserLogout(struct DFITCUserLogoutInfoRtnField * pUserLogoutInfoRtn, struct DFITCErrorRtnField * pErrorInfo){};
	//�µ�
	virtual void OnRspInsertOrder(struct DFITCOrderRspDataRtnField * pOrderRtn, struct DFITCErrorRtnField * pErrorInfo);
	virtual void OnRtnOrder(struct DFITCOrderRtnField * pRtnOrderData);

	virtual void OnRspQryOrderInfo(struct DFITCOrderCommRtnField * pRtnOrderData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};

	//����
	virtual void OnRspCancelOrder(struct DFITCOrderRspDataRtnField *pOrderCanceledRtn, struct DFITCErrorRtnField *pErrorInfo);
	virtual void OnRtnCancelOrder(struct DFITCOrderCanceledRtnField * pCancelOrderData){};

	//�ɽ��ر�
	virtual void OnRspQryMatchInfo(struct DFITCMatchedRtnField * pRtnMatchData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};
	virtual void OnRtnMatchedInfo(struct DFITCMatchRtnField * pRtnMatchData);

	//��λ
	virtual void OnRspQryPosition(struct DFITCPositionInfoRtnField * pPositionInfoRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};
	virtual void OnRspQryPositionDetail(struct DFITCPositionDetailRtnField * pPositionDetailRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};

	//�ʽ�
	virtual void OnRspCustomerCapital(struct DFITCCapitalInfoRtnField * pCapitalInfoRtn, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast);

	//��Լ��������
	virtual void OnRspQryExchangeInstrument(struct DFITCExchangeInstrumentRtnField * pInstrumentData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast);
	virtual void OnRspArbitrageInstrument(struct DFITCAbiInstrumentRtnField * pAbiInstrumentData, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast);
	virtual void OnRspQrySpecifyInstrument(struct DFITCInstrumentRtnField * pInstrument, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};

	////����
	virtual void OnRtnErrorMsg(struct DFITCErrorRtnField *pErrorInfo){};
	virtual void OnRtnExchangeStatus(struct DFITCExchangeStatusRtnField * pRtnExchangeStatusData){};

	virtual void OnRspResetPassword(struct DFITCResetPwdRspField * pResetPassword, struct DFITCErrorRtnField * pErrorInfo){};
	virtual void OnRspQryTradeCode(struct DFITCQryTradeCodeRtnField * pTradeCode, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};
	virtual void OnRspBillConfirm(struct DFITCBillConfirmRspField * pBillConfirm, struct DFITCErrorRtnField * pErrorInfo){};
	virtual void OnRspEquityComputMode(struct DFITCEquityComputModeRtnField * pEquityComputMode){};
	virtual void OnRspQryBill(struct DFITCQryBillRtnField *pQryBill, struct DFITCErrorRtnField * pErrorInfo, bool bIsLast){};
	virtual void OnRspConfirmProductInfo(struct DFITCProductRtnField * pProductRtnData){};
	virtual void OnRspTradingDay(struct DFITCTradingDayRtnField * pTradingDayRtnData){};

	// ������
	virtual void OnRspQuoteSubscribe(struct DFITCQuoteSubscribeRspField * pRspQuoteSubscribeData);
	virtual void OnRtnQuoteSubscribe(struct DFITCQuoteSubscribeRtnField * pRtnQuoteSubscribeData);
	virtual void OnRspQuoteInsert(struct DFITCQuoteRspField * pRspQuoteData, struct DFITCErrorRtnField * pErrorInfo){};
	virtual void OnRtnQuote(struct DFITCQuoteRtnField * pRtnQuoteData){};
	virtual void OnRspQuoteCancel(struct DFITCQuoteRspField * pRspQuoteCanceledData, struct DFITCErrorRtnField * pErrorInfo){};
	virtual void OnRtnQuoteCancel(struct DFITCQuoteCanceledRtnField * pRtnQuoteCanceledData){};

private:
	atomic<long>				m_lRequestID;			//����ID,�ñ�������

	DFITCUserLoginInfoRtnField	m_RspUserLogin;			//���صĵ�¼�ɹ���Ӧ��Ŀǰ���ô��ڳ�Ա���б�����������
	OrderIDType					m_orderInsert_Id;

	mutex						m_csOrderRef;
	int							m_nMaxOrderRef;			//�������ã��������ֱ�������������

	DFITCTraderApi*				m_pApi;					//����API
	
	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;
	int							m_nSleep;


	unordered_map<string, OrderField*>				m_id_platform_order;
	unordered_map<string, DFITCOrderRtnField*>		m_id_api_order;
	unordered_map<string, string>					m_sysId_orderId;

	//unordered_map<string, QuoteField*>				m_id_platform_quote;
	unordered_map<string, DFITCQuoteRtnField*>		m_id_api_quote;
	unordered_map<string, string>					m_sysId_quoteId;

	CMsgQueue*					m_msgQueue;				//��Ϣ����ָ��
	CMsgQueue*					m_msgQueue_Query;
	void*						m_pClass;
};

