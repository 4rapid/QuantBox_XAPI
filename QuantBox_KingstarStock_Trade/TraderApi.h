#pragma once

#include "../include/Kingstar_Stock/SPXAPI.h"
#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../include/Kingstar_Stock/win64/thosttraderapi.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/Kingstar_Stock/win32/SPXAPI.lib")
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

struct OrderFieldEx
{
	OrderField Field;
	TCustNoType         cust_no;                    ///< �ͻ���
	TMarketCodeType     market_code;                ///< �г����루�������������룩
	THolderNoType       holder_acc_no;              ///< �ɶ��ʺ�
	TBatchNoType        batch_no;                   ///< ����
	TContractNoType     order_no;                   ///< ��ˮ��
};

class CMsgQueue;

class CTraderApi
{
	//�������ݰ�����
	enum RequestType
	{
		E_Init,
		E_ReqUserLoginField,


		E_SettlementInfoConfirmField,
		E_QryInstrumentField,
		E_InputOrderField,
		E_InputOrderActionField,
		E_InputQuoteField,
		E_InputQuoteActionField,
		E_ParkedOrderField,
		E_QryTradingAccountField,
		E_QryInvestorPositionField,
		E_QryInvestorPositionDetailField,
		E_QryInstrumentCommissionRateField,
		E_QryInstrumentMarginRateField,
		E_QryDepthMarketDataField,
		E_QrySettlementInfoField,
		E_QryOrderField,
		E_QryTradeField,
		E_QryQuoteField,
	};

public:
	static CTraderApi * pThis;

	CTraderApi(void);
	virtual ~CTraderApi(void);

	void Register(void* pCallback, void* pClass);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo,
		int count);
	void Disconnect();

	int ReqOrderInsert(
		OrderField* pOrder,
		int count,
		OrderIDType* pInOut);

	//char* ReqParkedOrderInsert(int OrderRef,
	//	OrderField* pOrder1,
	//	OrderField* pOrder2);

	int ReqOrderAction(OrderIDType* szId, int count, OrderIDType* pOutput);
	int ReqOrderAction(OrderFieldEx *pOrder, int count, OrderIDType* pOutput);

	//char* ReqQuoteInsert(
	//	int QuoteRef,
	//	QuoteField* pQuote);

	//int ReqQuoteAction(CThostFtdcQuoteField *pQuote);
	////int ReqQuoteAction(const string& szId);

	//void ReqQryTradingAccount();
	//void ReqQryInvestorPosition(const string& szInstrumentId, const string& szExchange);
	//void ReqQryInvestorPositionDetail(const string& szInstrumentId);
	void ReqQryInstrument(const string& szInstrumentId, const string& szExchange);
	//void ReqQryInstrumentCommissionRate(const string& szInstrumentId);
	////void ReqQryInstrumentMarginRate(const string& szInstrumentId,TThostFtdcHedgeFlagType HedgeFlag = THOST_FTDC_HF_Speculation);
	//void ReqQryDepthMarketData(const string& szInstrumentId);
	//void ReqQrySettlementInfo(const string& szTradingDay);

	void ReqQryOrder(TCustNoType cust_no, TSecCodeType sec_code);
	void ReqQryTrade(TCustNoType cust_no, TSecCodeType sec_code);
	//void ReqQryQuote();

	
private:
	static void __stdcall OnReadPushData(ETX_APP_FUNCNO FuncNO, void* pEtxPushData);
	void _OnReadPushData(ETX_APP_FUNCNO FuncNO, void* pEtxPushData);

	friend void* __stdcall Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	virtual void QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	
	int _Init();

	//��¼����
	void ReqUserLogin();
	int _ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	int _ReqQryOrder(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	int _ReqQryTrade(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);


	void OnPST16203PushData(PST16203PushData pEtxPushData);
	void OnPST16204PushData(PST16204PushData pEtxPushData);

	

	//����Ƿ����
	bool IsErrorRspInfo(STRspMsg *pRspInfo, int nRequestID, bool bIsLast);//����Ϣ���������Ϣ
	bool IsErrorRspInfo(STRspMsg *pRspInfo);//�������Ϣ

	////����
	//virtual void OnFrontConnected();
	//virtual void OnFrontDisconnected(int nReason);

	////��֤
	//virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	////�µ�
	//virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

	////����
	//virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

	////�����ر�
	//virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

	////�ɽ��ر�
	//virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

	////����¼��
	//virtual void OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo);
	//virtual void OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRtnQuote(CThostFtdcQuoteField *pQuote);

	////���۳���
	//virtual void OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo);

	////��λ
	//virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	////�ʽ�
	//virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	////��Լ��������
	//virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	////��ѯ������Ӧ
	//virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	////�����ѯͶ���߽�������Ӧ
	//virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	////����
	//virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus);

private:
	atomic<int>					m_lRequestID;			//����ID,�ñ�������

	//CThostFtdcRspUserLoginField m_RspUserLogin;			//���صĵ�¼�ɹ���Ӧ��Ŀǰ���ô��ڳ�Ա���б�����������
	
	OrderIDType					m_orderInsert_Id;

	mutex						m_csOrderRef;
	int							m_nMaxOrderRef;			//�������ã��������ֱ�������������

	STRspMsg					m_err_msg;
	APIHandle					m_pApi;					//����API


	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;

	int							m_nSleep;

	unordered_map<string, OrderFieldEx*>				m_id_platform_order;
	//unordered_map<string, STOrderInfo*>				m_id_api_order;
	//unordered_map<string, string>					m_sysId_orderId;//�ɽ��ر�ʱʹ���ҵ�ԭ����

	//unordered_map<string, QuoteField*>				m_id_platform_quote;
	//unordered_map<string, CThostFtdcQuoteField*>		m_id_api_quote;
	//unordered_map<string, string>					m_sysId_quoteId;

	//unordered_map<string, PositionField*>			m_id_platform_position;

	unordered_map<string, string>					m_cust_acc_no;

	CMsgQueue*					m_msgQueue;				//��Ϣ����ָ��
	CMsgQueue*					m_msgQueue_Query;		//������Ϣ����ָ��
	CMsgQueue*					m_msgQueue_Order;		//������Ϣ����ָ��

	UserInfoField*				m_pUserInfos;
	int							m_UserInfo_Pos;
	int							m_UserInfo_Count;

	STOrderCancel				m_temp_ordercancel;

	void*						m_pClass;
};

