#pragma once

#include "../include/DFITC_L2/DFITCL2Api.h"

#include "../include/ApiStruct.h"

#ifdef _WIN64
#pragma comment(lib, "../lib/QuantBox_Queue_x64.lib")
#else
#pragma comment(lib, "../include/DFITC_L2/win32/level2Api.lib")
#pragma comment(lib, "../lib/QuantBox_Queue_x86.lib")
#endif

#include <set>
#include <string>
#include <atomic>
#include <mutex>
#include <map>
#include <list>
#include <thread>

using namespace std;
using namespace DFITC_L2;

class CMsgQueue;

class CLevel2UserApi :public DFITCL2Spi
{
	//�������ݰ�����
	enum RequestType
	{
		E_Init,
		E_UserLoginField,
	};

public:
	CLevel2UserApi(void);
	virtual ~CLevel2UserApi(void);

	void Register(void* pCallback, void* pClass);

	void Connect(const string& szPath,
		ServerInfoField* pServerInfo,
		UserInfoField* pUserInfo);
	void Disconnect();

	void Subscribe(const string& szInstrumentIDs, const string& szExchageID);
	void Unsubscribe(const string& szInstrumentIDs, const string& szExchageID);

	void SubscribeAll();
	void UnsubscribeAll();

private:
	friend void* __stdcall Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);
	void QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);

	int _Init();
	//��¼����
	void ReqUserLogin();
	int _ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3);


	//��������
	void Subscribe(const set<string>& instrumentIDs, const string& szExchageID);
	void Unsubscribe(const set<string>& instrumentIDs, const string& szExchageID);

	virtual void OnConnected();
	virtual void OnDisconnected(int nReason);
	virtual void OnRspUserLogin(struct ErrorRtnField * pErrorField);

	virtual void OnRspSubscribeMarketData(struct ErrorRtnField * pErrorField);
	virtual void OnRspUnSubscribeMarketData(struct ErrorRtnField * pErrorField);
	virtual void OnRspSubscribeAll(struct ErrorRtnField * pErrorField);
	virtual void OnRspUnSubscribeAll(struct ErrorRtnField * pErrorField);

	virtual void OnBestAndDeep(MDBestAndDeep * const pQuote);
	virtual void OnArbi(MDBestAndDeep * const pQuote);
	virtual void OnTenEntrust(MDTenEntrust * const pQuote);
	virtual void OnRealtime(MDRealTimePrice * const pQuote);
	virtual void OnOrderStatistic(MDOrderStatistic * const pQuote);
	virtual void OnMarchPrice(MDMarchPriceQty * const pQuote);

	virtual void OnHeartBeatLost() { }
	//����Ƿ����
	bool IsErrorRspInfo_Output(struct ErrorRtnField * pErrorField);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(struct ErrorRtnField * pErrorField); //�������Ϣ

private:
	mutex						m_csMapInstrumentIDs;

	atomic<long>				m_lRequestID;			//����ID��ÿ������ǰ����

	set<string>					m_setInstrumentIDs;

	DFITCL2Api*					m_pApi;					//����API
	
	string						m_szPath;				//���������ļ���·��
	ServerInfoField				m_ServerInfo;
	UserInfoField				m_UserInfo;

	int							m_nSleep;
	
	CMsgQueue*					m_msgQueue;				//��Ϣ����ָ��
	CMsgQueue*					m_msgQueue_Query;
	void*						m_pClass;
};

