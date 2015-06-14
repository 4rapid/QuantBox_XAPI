#pragma once

#include "../include/Femas/USTPFtdcTraderApi.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/Femas/win64/USTPtraderapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/Femas/win32/USTPtraderapi.lib")
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

class CMsgQueue;

class CTraderApi :
	public CUstpFtdcTraderSpi
{
	//�������ݰ�����
	enum RequestType
	{
		E_Init,

		E_ReqUserLoginField,
		E_QryUserInvestorField,
		E_QryInstrumentField,
		E_InputOrderField,
		//E_InputOrderActionField,
		E_InputQuoteField,
		//E_InputQuoteActionField,
		E_ParkedOrderField,
		E_QryInvestorAccountField,
		E_QryInvestorPositionField,
		E_QryInvestorPositionDetailField,
		E_QryInvestorFeeField,
		E_QryInvestorMarginField,
		E_QryDepthMarketDataField,
		E_QrySettlementInfoField,
		E_QryOrderField,
		E_QryTradeField,
		E_OrderActionField,
		E_QuoteActionField,
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
	int ReqOrderAction(CUstpFtdcOrderField *pOrder, int count, OrderIDType* pOutput);

	char* ReqQuoteInsert(
		int QuoteRef,
		QuoteField* pQuote);

	//int ReqQuoteAction(CUstpFtdcRtnQuoteField *pQuote);
	//int ReqQuoteAction(const string& szId);

	void ReqQryInvestorAccount();
	void ReqQryInvestorPosition(const string& szInstrumentId);
	void ReqQryInstrument(const string& szInstrumentId, const string& szExchange);
	void ReqQryInvestorFee(const string& szInstrumentId);
	//void ReqQryInvestorMargin(const string& szInstrumentId, TThostFtdcHedgeFlagType HedgeFlag = THOST_FTDC_HF_Speculation);

	void ReqQryOrder();
	void ReqQryTrade();
	void ReqQryQuote();

private:
	friend void* __stdcall Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	void QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	
	void Clear();

	int _Init();

	void ReqUserLogin();
	int _ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	void ReqQryUserInvestor();
	int _ReqQryUserInvestor(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	int _ReqQryInstrument(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	int _ReqQryInvestorAccount(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	int _ReqQryInvestorPosition(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	int _ReqQryOrder(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	int _ReqQryTrade(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	int _ReqQryQuote(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	void OnOrder(CUstpFtdcOrderField *pOrder);
	void OnTrade(CUstpFtdcTradeField *pTrade);
	//void OnQuote(CUstpFtdcRtnQuoteField *pQuote);

	//����Ƿ����
	bool IsErrorRspInfo(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);//����Ϣ���������Ϣ
	bool IsErrorRspInfo(CUstpFtdcRspInfoField *pRspInfo);//�������Ϣ

	//����
	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);

	//��֤
	virtual void OnRspUserLogin(CUstpFtdcRspUserLoginField *pRspUserLogin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryUserInvestor(CUstpFtdcRspUserInvestorField *pRspUserInvestor, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//�µ�
	virtual void OnRspOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderInsert(CUstpFtdcInputOrderField *pInputOrder, CUstpFtdcRspInfoField *pRspInfo);

	//����
	virtual void OnRspOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderAction(CUstpFtdcOrderActionField *pOrderAction, CUstpFtdcRspInfoField *pRspInfo);

	//�����ر�
	virtual void OnRspQryOrder(CUstpFtdcOrderField *pOrder, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnOrder(CUstpFtdcOrderField *pOrder);

	//�ɽ��ر�
	virtual void OnRspQryTrade(CUstpFtdcTradeField *pTrade, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnTrade(CUstpFtdcTradeField *pTrade);

	////����¼��
	//virtual void OnRspQuoteInsert(CUstpFtdcInputQuoteField *pInputQuote, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnQuoteInsert(CUstpFtdcInputQuoteField *pInputQuote, CUstpFtdcRspInfoField *pRspInfo);
	//virtual void OnRtnQuote(CUstpFtdcRtnQuoteField *pRtnQuote);

	////���۳���
	//virtual void OnRspQuoteAction(CUstpFtdcQuoteActionField *pQuoteAction, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnQuoteAction(CUstpFtdcQuoteActionField *pQuoteAction, CUstpFtdcRspInfoField *pRspInfo);

	//��λ
	virtual void OnRspQryInvestorPosition(CUstpFtdcRspInvestorPositionField *pRspInvestorPosition, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	//�ʽ�
	virtual void OnRspQryInvestorAccount(CUstpFtdcRspInvestorAccountField *pRspInvestorAccount, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//��Լ��������
	virtual void OnRspQryInstrument(CUstpFtdcRspInstrumentField *pRspInstrument, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInvestorMargin(CUstpFtdcInvestorMarginField *pInvestorMargin, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorFee(CUstpFtdcInvestorFeeField *pInvestorFee, CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	//����
	virtual void OnRspError(CUstpFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnInstrumentStatus(CUstpFtdcInstrumentStatusField *pInstrumentStatus);

	//ѯ�ۻر�
	//virtual void OnRtnForQuote(CUstpFtdcReqForQuoteField *pReqForQuote);

private:
	atomic<int>					m_lRequestID;			//����ID,�ñ�������

	RspUserLoginField			m_RspUserLogin__;
	CUstpFtdcRspUserLoginField	m_RspUserLogin;			//���صĵ�¼�ɹ���Ӧ��Ŀǰ���ô��ڳ�Ա���б�����������
	CUstpFtdcRspUserInvestorField m_RspUserInvestor;

	OrderIDType					m_orderInsert_Id;
	OrderIDType					m_orderAction_Id;

	mutex						m_csOrderRef;
	long long					m_nMaxOrderRef;			//�������ã��������ֱ�������������

	CUstpFtdcTraderApi*			m_pApi;					//����API

	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;

	int							m_nSleep;


	unordered_map<string, OrderField*>				m_id_platform_order;
	unordered_map<string, CUstpFtdcOrderField*>		m_id_api_order;
	//unordered_map<string, string>					m_sysId_orderId;

	unordered_map<string, QuoteField*>				m_id_platform_quote;
	//unordered_map<string, CUstpFtdcRtnQuoteField*>		m_id_api_quote;
	//unordered_map<string, string>					m_sysId_quoteId;

	CMsgQueue*					m_msgQueue;				//��Ϣ����ָ��
	CMsgQueue*					m_msgQueue_Query;
	void*						m_pClass;
};

