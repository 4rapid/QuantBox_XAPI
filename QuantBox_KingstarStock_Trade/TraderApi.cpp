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

CTraderApi* CTraderApi::pThis = nullptr;

void __stdcall CTraderApi::OnReadPushData(ETX_APP_FUNCNO FuncNO, void* pEtxPushData)
{
	// �������һ��dll����ʵ��������ˣ���Ϊʹ����static����
	pThis->_OnReadPushData(FuncNO, pEtxPushData);
}

//�ͻ���ʵ�����ͻص�����
void CTraderApi::_OnReadPushData(ETX_APP_FUNCNO FuncNO, void* pEtxPushData)
{
	switch (FuncNO)
	{
	case ETX_16203:
		OnPST16203PushData((PST16203PushData)pEtxPushData);
		break;
	case ETX_16204:
		OnPST16204PushData((PST16204PushData)pEtxPushData);
		break;
	default:
	{
			   ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

			   pField->ErrorID = FuncNO;
			   sprintf(pField->ErrorMsg, "�޷�ʶ�����������[%d]", FuncNO);

			   m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, true, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
		break;
	}
}

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
	case E_ReqUserLoginField:
		iRet = _ReqUserLogin(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case E_QryOrderField:
		iRet = _ReqQryOrder(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case E_QryTradeField:
		iRet = _ReqQryTrade(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
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

int CTraderApi::_Init()
{
	//���ص�ַ�������Ϣ�ɿͻ��������ã�����ʵ��������ã��������ã�
	//����������Ϣ

	char szPost[20] = { 0 };
	_itoa(m_ServerInfo.Port, szPost, 10);

	SPX_API_SetParam(MAINSERVER_IP, m_ServerInfo.Address, &m_err_msg);
	SPX_API_SetParam(MAINSERVER_PORT, szPost, &m_err_msg);
	//SPX_API_SetParam(BACKSERVER_IP, "127.0.0.1", &m_err_msg);
	//SPX_API_SetParam(BACKSERVER_PORT, "17990", &m_err_msg);
	//���ô�����Ϣ
	//SPX_API_SetParam(PROXY_TYPE, "1", &m_err_msg);
	//SPX_API_SetParam(PROXY_IP, "127.0.0.1", &m_err_msg);
	//SPX_API_SetParam(PROXY_PORT, "9999", &m_err_msg);
	//SPX_API_SetParam(PROXY_USER, "", &m_err_msg);
	//SPX_API_SetParam(PROXY_PASS, "", &m_err_msg);


	STInitPara init_para;
	init_para.pOnReadPushData = OnReadPushData;
	init_para.bWriteLog = false;//����п��ܵ���Ҫ�ù���ԱȨ�޲��ܶ�дĿ¼
	init_para.emLogLevel = LL_INFO;
	init_para.nTimeOut = 60000;

	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Uninitialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	bool bRet = SPX_API_Initialize(&init_para, &m_err_msg);

	RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

	//����ʧ�ܷ��ص���Ϣ��ƴ�Ӷ��ɣ���Ҫ��Ϊ��ͳһ���
	pField->ErrorID = m_err_msg.error_no;
	strcpy(pField->ErrorMsg, m_err_msg.msg);
	ConnectionStatus status;

	do
	{
		if (!bRet)
		{
			status = ConnectionStatus::Disconnected;
			break;
		}
		status = ConnectionStatus::Initialized;
		// ���ں��滹Ҫ�ٴ�һ�Σ�Ϊ�˷�ֹ����ͷţ�����ʹ��Copy
		m_msgQueue->Input_Copy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, status, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

		m_pApi = SPX_API_CreateHandle(&m_err_msg);
		if (m_pApi == nullptr)
		{
			pField->ErrorID = m_err_msg.error_no;
			strcpy(pField->ErrorMsg, m_err_msg.msg);
			status = ConnectionStatus::Disconnected;
			break;
		}

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		// ��¼��һ��
		ReqUserLogin();

		return 0;
	} while (false);

	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, status, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

	return 0;
}

void CTraderApi::ReqUserLogin()
{
	if (m_UserInfo_Pos >= m_UserInfo_Count)
		return;

	STTraderLogin* pBody = (STTraderLogin*)m_msgQueue_Query->new_block(sizeof(STTraderLogin));

	strncpy(pBody->cust_no, m_pUserInfos[m_UserInfo_Pos].UserID, sizeof(TCustNoType));
	strncpy(pBody->cust_pwd, m_pUserInfos[m_UserInfo_Pos].Password, sizeof(TCustPwdType));

	m_msgQueue_Query->Input_NoCopy(RequestType::E_ReqUserLoginField, m_msgQueue_Query, this, 0, 0,
		pBody, sizeof(STTraderLogin), nullptr, 0, nullptr, 0);
}

int CTraderApi::_ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int row_num = 0;
	STTraderLoginRsp *p_login_rsp = nullptr;
	STTraderLogin* p_login_req = (STTraderLogin*)ptr1;

	bool bRet = SPX_API_Login(m_pApi, p_login_req, &p_login_rsp, &row_num, &m_err_msg);

	if (bRet && m_err_msg.error_no == 0)
	{
		if (row_num <= 0)
		{
			RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

			pField->ErrorID = m_err_msg.error_no;
			sprintf(pField->SessionID, "%s:", p_login_req->cust_no);
			pField->ErrorID = -1;
			sprintf(pField->ErrorMsg, "%s:���ؽ��Ϊ��", p_login_req->cust_no);

			m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Logined, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
		}
		else if (p_login_rsp != nullptr)
		{
			for (int i = 0; i<row_num; i++)
			{
				RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

				pField->TradingDay = GetDate(p_login_rsp[i].tx_date);
				sprintf(pField->SessionID, "%s:%c:%s", p_login_req->cust_no, p_login_rsp[i].market_code, p_login_rsp[i].holder_acc_no);
				sprintf(pField->InvestorName, "%s", p_login_rsp[i].cust_name);

				char buf[50] = { 0 };
				sprintf(buf, "%s:%c", p_login_req->cust_no, p_login_rsp[i].market_code);
				m_cust_acc_no.insert(pair<string, string>(buf, p_login_rsp[i].holder_acc_no));

				m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Logined, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

				// ��¼ʱ��ѯ���˺��µ����б�����ɽ�
				// �˴�ע�⣬����˺Ų�ѯʱ�ᵼ��ÿ�ζ���ȫ����ѯ
				ReqQryOrder(p_login_req[0].cust_no,"");
				ReqQryTrade(p_login_req[0].cust_no,"");
			}
		}
	}
	else
	{
		RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

		pField->ErrorID = m_err_msg.error_no;
		strcpy(pField->ErrorMsg, m_err_msg.msg);

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}

	// ��¼��һ���˺�
	++m_UserInfo_Pos;
	ReqUserLogin();

	return 0;
}

CTraderApi::CTraderApi(void)
{
	m_pApi = nullptr;
	m_lRequestID = 0;
	m_nSleep = 1;

	// �Լ�ά��������Ϣ����
	m_msgQueue = new CMsgQueue();
	m_msgQueue_Query = new CMsgQueue();

	m_msgQueue_Query->Register(Query,this);
	m_msgQueue_Query->StartThread();

	pThis = this;
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

	m_msgQueue_Query->Register(Query,this);
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

bool CTraderApi::IsErrorRspInfo(STRspMsg *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bRet = ((pRspInfo) && (pRspInfo->error_no != 0));
	if (bRet)
	{
		ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

		pField->ErrorID = pRspInfo->error_no;
		strcpy(pField->ErrorMsg, pRspInfo->msg);

		m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
	return bRet;
}

bool CTraderApi::IsErrorRspInfo(STRspMsg *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->error_no != 0));

	return bRet;
}

void CTraderApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo,
	int count)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));

	m_pUserInfos = (UserInfoField*)(new char[sizeof(UserInfoField)*count]);
	memcpy(m_pUserInfos, pUserInfo, sizeof(UserInfoField)*count);

	m_UserInfo_Pos = 0;
	m_UserInfo_Count = count;

	m_msgQueue_Query->Input_NoCopy(E_Init, m_msgQueue_Query, this, 0, 0, nullptr, 0, nullptr, 0, nullptr, 0);
}

void CTraderApi::Disconnect()
{
	if (m_msgQueue_Query)
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue_Query->Register(nullptr,nullptr);
		m_msgQueue_Query->Clear();
		delete m_msgQueue_Query;
		m_msgQueue_Query = nullptr;
	}

	if(m_pApi)
	{
		SPX_API_DestroyHandle(&m_pApi, &m_err_msg);
		m_pApi = nullptr;

		// ȫ����ֻ�����һ��
		m_msgQueue->Clear();
		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		// ��������
		m_msgQueue->Process();
	}
	SPX_API_Finalize(&m_err_msg);

	if (m_msgQueue)
	{
		m_msgQueue->StopThread();
		m_msgQueue->Register(nullptr,nullptr);
		m_msgQueue->Clear();
		delete m_msgQueue;
		m_msgQueue = nullptr;
	}
}

void BuildOrder(OrderField* pIn, PSTOrder pOut)
{
	strncpy(pOut->sec_code, pIn->InstrumentID, sizeof(TSecCodeType));
	OrderField_2_TBSType(pIn, pOut);
	OrderField_2_TMarketOrderFlagType(pIn, pOut);
	pOut->price = pIn->Price;
	pOut->order_vol = pIn->Qty;
	//pOut->order_prop;
}

int CTraderApi::ReqOrderInsert(
	OrderField* pOrder,
	int count,
	OrderIDType* pInOut)
{
	if (count < 1)
		return 0;

	// ����������µ��ǲ�ͬ�г�����ͬ�˺���ô�죿��Ҫ�Լ��ֳ�������
	// ����µ�������ģʽ���Ƿ�������ɱ�ţ������޷�����ӳ��
	int row_num = 0;
	STOrder *p_order_req = new STOrder[count];
	STOrderRsp *p_order_rsp = NULL;

	memset(p_order_req, 0, sizeof(STOrder)*count);
	memset(pInOut, 0, sizeof(OrderIDType)*count);

	for (int i = 0; i < count;++i)
	{
		BuildOrder(&pOrder[i], &p_order_req[i]);
	}

	//char scust_no[11] = "0000000013";
	char smarket_code[2] = {0};// ��εõ���
	//char sholder_acc_no[15] = "A780891297";
	char sorder_type[2] = "0";// ���ָ����

	// �ȿ��Ƿ�ָ�����˺ţ����û��ָ������Ĭ�ϵ�һ��
	if (strlen(pOrder[0].ClientID) == 0)
	{
		strncpy(pOrder[0].ClientID, m_UserInfo.UserID, sizeof(UserIDType));
	}

	// ���Ƶõ��г����룬���ܲ�׼
	smarket_code[0] = OrderField_2_TMarketCodeType(&pOrder[0]);

	// �ӵ�¼����б��в鵽��Щ�˺ſ���
	if (strlen(pOrder[0].Account) == 0)
	{
		char buf[50] = { 0 };
		sprintf(buf, "%s:%s", pOrder[0].ClientID, smarket_code);
		unordered_map<string, string>::iterator it = m_cust_acc_no.find(buf);
		if (it == m_cust_acc_no.end())
		{
		}
		else
		{
			strncpy(pOrder[0].Account, it->second.c_str(), sizeof(AccountIDType));
		}
	}

	bool bRet = SPX_API_Order(m_pApi, pOrder[0].ClientID, smarket_code, pOrder[0].Account, sorder_type, p_order_req, &p_order_rsp, count, &row_num, &m_err_msg);

	if (bRet && m_err_msg.error_no == 0)
	{
		if (p_order_rsp != NULL)
		{
			for (int i = 0; i<row_num; i++)
			{
				pOrder[i].ErrorID = p_order_rsp[i].error_no;
				strncpy(pOrder[i].Text, p_order_rsp[i].err_msg, sizeof(ErrorMsgType));

				if (p_order_rsp[i].error_no == 0)
				{
					pOrder[i].ExecType = ExecType::ExecNew;
					pOrder[i].Status = OrderStatus::New;

					sprintf(pOrder[i].ID, "%d", p_order_rsp[i].order_no);
					strncpy(pInOut[i], pOrder[i].ID, sizeof(OrderIDType));

					{
						OrderFieldEx* pField = (OrderFieldEx*)m_msgQueue->new_block(sizeof(OrderFieldEx));
						memcpy(&pField->Field, &pOrder[i], sizeof(OrderField));

						strncpy(pField->cust_no, pOrder[0].ClientID, sizeof(TCustNoType));
						strncpy(pField->holder_acc_no, pOrder[0].Account, sizeof(THolderNoType));
						pField->batch_no = p_order_rsp[i].batch_no;
						pField->order_no = p_order_rsp[i].order_no;
						pField->market_code = smarket_code[0];

						m_id_platform_order.insert(pair<string, OrderFieldEx*>(pInOut[i], pField));
					}
				}
				else
				{
					// ������û�����ɣ�����Ҫ���ض�����
					sprintf(pOrder[i].ID, "XAPI:%d", time(nullptr)*100 + i);
					strncpy(pInOut[i], pOrder[i].ID, sizeof(OrderIDType));

					pOrder[i].ExecType = ExecType::ExecRejected;
					pOrder[i].Status = OrderStatus::Rejected;
				}

				// ���µ�ʱ�������������¼�
				// �������¼��Ⱥ������������������أ�������������������Ԥ�ƵĴ���
				m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i], sizeof(OrderField), nullptr, 0, nullptr, 0);
			}
		}
		else
		{
			for (int i = 0; i < count; ++i)
			{
				// ������û�����ɣ�����Ҫ���ض�����
				sprintf(pOrder[i].ID, "XAPI:%d", time(nullptr) * 100 + i);
				strncpy(pInOut[i], pOrder[i].ID, sizeof(OrderIDType));

				pOrder[i].ErrorID = m_err_msg.error_no;
				strcpy(pOrder[i].Text, "���ؽ��Ϊ��");
				pOrder[i].ExecType = ExecType::ExecRejected;
				pOrder[i].Status = OrderStatus::Rejected;

				m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i], sizeof(OrderField), nullptr, 0, nullptr, 0);
			}
		}
	}
	else
	{
		// �����ˣ�ȫ���
		for (int i = 0; i < count; ++i)
		{
			// ������û�����ɣ�����Ҫ���ض�����
			sprintf(pOrder[i].ID, "XAPI:%d", time(nullptr) * 100 + i);
			strncpy(pInOut[i], pOrder[i].ID, sizeof(OrderIDType));

			pOrder[i].ErrorID = m_err_msg.error_no;
			strcpy(pOrder[i].Text, m_err_msg.msg);
			pOrder[i].ExecType = ExecType::ExecRejected;
			pOrder[i].Status = OrderStatus::Rejected;

			m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i], sizeof(OrderField), nullptr, 0, nullptr, 0);
		}
	}

	delete[] p_order_req;

	// Ӧ�������̣߳����пɳ�����ѯ
	ReqQryOrder(pOrder[0].ClientID,"");
	
	return 0;
}

int CTraderApi::ReqOrderAction(OrderIDType* szId, int count, OrderIDType* pOutput)
{
	OrderFieldEx *pOrder = new OrderFieldEx[count];

	for (int i = 0; i < count; ++i)
	{
		unordered_map<string, OrderFieldEx*>::iterator it = m_id_platform_order.find(szId[i]);
		if (it == m_id_platform_order.end())
		{
			// û�ҵ���ô�죿��һ����û�ҵ���ô�죿
			if (i>0)
				memcpy(&pOrder[i], &pOrder[i-1], sizeof(OrderFieldEx));

			pOrder[i].order_no = atoi(szId[i]);
		}
		else
		{
			memcpy(&pOrder[i], it->second, sizeof(OrderFieldEx));
		}
	}

	int nRet = ReqOrderAction(pOrder, count, pOutput);
	delete[] pOrder;
	return nRet;
}

void BuildCancelOrder(OrderFieldEx* pIn, STOrderCancel* pOut)
{
	strcpy(pOut->cust_no,pIn->cust_no);
	pOut->market_code = pIn->market_code;
	pOut->ordercancel_type = 1;
	pOut->order_no = pIn->order_no;
}

int CTraderApi::ReqOrderAction(OrderFieldEx *pOrder, int count, OrderIDType* pOutput)
{
	int row_num = 0;

	STOrderCancel *p_ordercancel_req = new STOrderCancel[count];
	STOrderCancelRsp *p_ordercancel_rsp = NULL;

	for (int i = 0; i < count; ++i)
	{
		BuildCancelOrder(&pOrder[i], &p_ordercancel_req[i]);
	}

	bool bRet = SPX_API_OrderCancel(m_pApi, p_ordercancel_req, &p_ordercancel_rsp, count, &row_num, &m_err_msg);

	if (bRet && m_err_msg.error_no == 0)
	{
		if (p_ordercancel_rsp != NULL)
		{
			for (int i = 0; i<row_num; i++)
			{
				if (p_ordercancel_rsp[i].error_no == 0)
				{
					// ���ǳ����ɹ����ǳ���ʧ�����أ�
					memset(pOutput[i], 0, sizeof(OrderIDType));
					
					pOrder[i].Field.ErrorID = p_ordercancel_rsp[i].error_no;
					strncpy(pOrder[i].Field.Text, p_ordercancel_rsp[i].err_msg, sizeof(ErrorMsgType));
					pOrder[i].Field.ExecType = ExecType::ExecCancelReject;
					pOrder[i].Field.Status = OrderStatus::Rejected;

					m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i].Field, sizeof(OrderField), nullptr, 0, nullptr, 0);
				}
				else
				{
					memset(pOutput[i], 0, sizeof(OrderIDType));
					//sprintf(pOutput[i],"%d",p_ordercancel_rsp[i].error_no);

					pOrder[i].Field.ErrorID = p_ordercancel_rsp[i].error_no;
					strncpy(pOrder[i].Field.Text, p_ordercancel_rsp[i].err_msg, sizeof(ErrorMsgType));
					pOrder[i].Field.ExecType = ExecType::ExecCancelReject;
					// �����ܾ���״̬Ҫ���ϴε�
					//pOrder[i].Field.Status = OrderStatus::Rejected;

					m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i].Field, sizeof(OrderField), nullptr, 0, nullptr, 0);
				}
			}
		}
		else
		{
			for (int i = 0; i < count; ++i)
			{
				//sprintf(pOutput[i], "%d", 0);
				memset(pOutput[i], 0, sizeof(OrderIDType));

				pOrder[i].Field.ErrorID = 0;
				strcpy(pOrder[i].Field.Text, "���ؽ��Ϊ��");
				pOrder[i].Field.ExecType = ExecType::ExecCancelReject;
				//pOrder[i].Field.Status = OrderStatus::Rejected;

				m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i].Field, sizeof(OrderField), nullptr, 0, nullptr, 0);
			}
		}
	}
	else
	{
		// �����ˣ�ȫ���
		for (int i = 0; i < count; ++i)
		{
			//sprintf(pOutput[i], "%d", m_err_msg.error_no);
			memset(pOutput[i], 0, sizeof(OrderIDType));

			pOrder[i].Field.ErrorID = m_err_msg.error_no;
			strcpy(pOrder[i].Field.Text, m_err_msg.msg);
			pOrder[i].Field.ExecType = ExecType::ExecCancelReject;
			//pOrder[i].Field.Status = OrderStatus::Rejected;

			m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pOrder[i].Field, sizeof(OrderField), nullptr, 0, nullptr, 0);
		}
	}

	delete[] p_ordercancel_req;

	return 0;
}

void CTraderApi::ReqQryOrder(TCustNoType cust_no, TSecCodeType sec_code)
{
	STQueryOrder* pField = (STQueryOrder*)m_msgQueue->new_block(sizeof(STQueryOrder));
	strcpy(pField->cust_no, cust_no);
	strcpy(pField->sec_code, sec_code);
	pField->market_code = '0';
	pField->order_no = 0;
	pField->query_type = 0;

	m_msgQueue_Query->Input_NoCopy(RequestType::E_QryOrderField, m_msgQueue_Query, this, 0, 0,
		pField, sizeof(STQueryOrder), nullptr, 0, nullptr, 0);
}

int CTraderApi::_ReqQryOrder(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int row_num = 0;

	STQueryOrder *p_qry_order_req = (STQueryOrder *)ptr1;
	STOrderInfo *p_qry_order_rsp = NULL;

	bool bRet = SPX_API_QueryOrder(m_pApi, p_qry_order_req, &p_qry_order_rsp, &row_num, &m_err_msg);

	if (bRet && m_err_msg.error_no == 0)
	{
		if (p_qry_order_rsp != NULL)
		{
			for (int i = 0; i<row_num; i++)
			{
				OrderIDType ID = { 0 };
				sprintf(ID,"%d",p_qry_order_rsp[i].order_no);

				OrderFieldEx* pField = nullptr;
				
				unordered_map<string, OrderFieldEx*>::iterator it = m_id_platform_order.find(ID);
				if (it == m_id_platform_order.end())
				{
					// �������ܲ����ڣ�Ҫ���л�ԭ
					pField = (OrderFieldEx*)m_msgQueue->new_block(sizeof(OrderFieldEx));

					strncpy(pField->cust_no, p_qry_order_req->cust_no, sizeof(TCustNoType));
					strncpy(pField->holder_acc_no, p_qry_order_rsp[i].holder_acc_no, sizeof(THolderNoType));
					pField->batch_no = p_qry_order_rsp[i].batch_no;
					pField->order_no = p_qry_order_rsp[i].order_no;
					pField->market_code = p_qry_order_rsp[i].market_code;

					pField->Field.Status = TOrderStatusType_2_OrderStatus(p_qry_order_rsp[i].order_status);
					pField->Field.ExecType = TOrderStatusType_2_ExecType(p_qry_order_rsp[i].order_status);
					pField->Field.Price = p_qry_order_rsp[i].price;
					pField->Field.Qty = p_qry_order_rsp[i].order_vol;
					pField->Field.CumQty = p_qry_order_rsp[i].done_vol;
					pField->Field.LeavesQty = p_qry_order_rsp[i].order_vol - p_qry_order_rsp[i].done_vol - p_qry_order_rsp[i].cancel_vol;
					pField->Field.Time = p_qry_order_rsp[i].order_time;
					strcpy(pField->Field.ID, ID);
					strcpy(pField->Field.InstrumentID, p_qry_order_rsp[i].sec_code);
					sprintf(pField->Field.ExchangeID, "%c", p_qry_order_rsp[i].market_code);

					m_id_platform_order.insert(pair<string, OrderFieldEx*>(ID, pField));
				}
				else
				{
					pField = it->second;

					pField->Field.Status = TOrderStatusType_2_OrderStatus(p_qry_order_rsp[i].order_status);
					pField->Field.ExecType = TOrderStatusType_2_ExecType(p_qry_order_rsp[i].order_status);

					pField->Field.Price = p_qry_order_rsp[i].price;
					pField->Field.Qty = p_qry_order_rsp[i].order_vol;
					pField->Field.CumQty = p_qry_order_rsp[i].done_vol;
					pField->Field.LeavesQty = p_qry_order_rsp[i].order_vol - p_qry_order_rsp[i].done_vol - p_qry_order_rsp[i].cancel_vol;
					pField->Field.Time = p_qry_order_rsp[i].order_time;
					strcpy(pField->Field.InstrumentID, p_qry_order_rsp[i].sec_code);
					sprintf(pField->Field.ExchangeID, "%c", p_qry_order_rsp[i].market_code);
					
				}
				
				m_msgQueue->Input_Copy(ResponeType::OnRtnOrder, m_msgQueue, m_pClass, 0, 0, &pField->Field, sizeof(OrderField), nullptr, 0, nullptr, 0);
			}
		}
		else
		{
			//cout << "���ؽ��Ϊ��" << endl;
			// ����Ϊ�գ���ʾ�Ѿ����һ�Σ�û���µ����ɣ����ô���
			// ������Ƕ��˺ţ�����ֶ�ÿ����ѯ���Զ����ȫ���ˣ��������α걻������,��δ���
			//return 0;
		}
	}
	else
	{
		//cout << "QueryOrder error:" << err_msg.error_no << err_msg.msg << endl;
		ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

		pField->ErrorID = m_err_msg.error_no;
		strcpy(pField->ErrorMsg, m_err_msg.msg);

		m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, true, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}

	return 0;
}

void CTraderApi::ReqQryTrade(TCustNoType cust_no, TSecCodeType sec_code)
{
	STQueryDone* pField = (STQueryDone*)m_msgQueue->new_block(sizeof(STQueryDone));
	strcpy(pField->cust_no, cust_no);
	strcpy(pField->sec_code, sec_code);
	pField->market_code = '0';
	pField->query_type = 0;

	m_msgQueue_Query->Input_NoCopy(RequestType::E_QryTradeField, m_msgQueue_Query, this, 0, 0,
		pField, sizeof(STQueryDone), nullptr, 0, nullptr, 0);
}

int CTraderApi::_ReqQryTrade(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int row_num = 0;

	STQueryDone *p_qry_done_req = (STQueryDone *)ptr1;
	STDoneInfo *p_qry_done_rsp = NULL;

	bool bRet = SPX_API_QueryDone(m_pApi, p_qry_done_req, &p_qry_done_rsp, &row_num, &m_err_msg);

	if (bRet && m_err_msg.error_no == 0)
	{
		if (p_qry_done_rsp != NULL)
		{
			//cout << "QueryOrder OK." << endl;
			for (int i = 0; i<row_num; i++)
			{
				//cout << p_qry_order_rsp[i].market_code << " " << p_qry_order_rsp[i].sec_code << " " << p_qry_order_rsp[i].batch_no << " " << p_qry_order_rsp[i].order_no << " " << p_qry_order_rsp[i].bs << " ";
				//cout << p_qry_order_rsp[i].order_date << " " << p_qry_order_rsp[i].price << " " << p_qry_order_rsp[i].order_vol << " " << p_qry_order_rsp[i].order_status << " " << p_qry_order_rsp[i].done_price << endl;
				TradeField* pField = (TradeField*)m_msgQueue->new_block(sizeof(TradeField));
				strcpy(pField->InstrumentID, p_qry_done_rsp[i].sec_code);
				sprintf(pField->ExchangeID, "%c", p_qry_done_rsp[i].market_code);

				pField->Side = TBSFLAG_2_OrderSide(p_qry_done_rsp[i].bs);
				pField->Qty = p_qry_done_rsp[i].done_vol;
				pField->Price = p_qry_done_rsp[i].done_price;
				//pField->OpenClose = TThostFtdcOffsetFlagType_2_OpenCloseType(pTrade->OffsetFlag);
				//pField->HedgeFlag = TThostFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);
				pField->Commission = 0;//TODO�������Ժ�Ҫ�������
				pField->Time = p_qry_done_rsp[i].done_time;
				strcpy(pField->TradeID, p_qry_done_rsp[i].done_no);
				sprintf(pField->ID, "%d", p_qry_done_rsp[i].order_no);

				m_msgQueue->Input_Copy(ResponeType::OnRtnTrade, m_msgQueue, m_pClass, 0, 0, pField, sizeof(TradeField), nullptr, 0, nullptr, 0);

				//OrderIDType orderSysId = { 0 };
				//sprintf(orderSysId, "%s:%s", pTrade->ExchangeID, pTrade->OrderSysID);
				//unordered_map<string, string>::iterator it = m_sysId_orderId.find(pField->ID);
				//if (it == m_sysId_orderId.end())
				//{
				//	// �˳ɽ��Ҳ�����Ӧ�ı���
				//	//assert(false);
				//}
				//else
				//{
				//	// �ҵ���Ӧ�ı���
				//	strcpy(pField->ID, it->second.c_str());

				//	

				//	//unordered_map<string, OrderField*>::iterator it2 = m_id_platform_order.find(it->second);
				//	//if (it2 == m_id_platform_order.end())
				//	//{
				//	//	// �˳ɽ��Ҳ�����Ӧ�ı���
				//	//	//assert(false);
				//	//}
				//	//else
				//	//{
				//	//	// ���¶�����״̬
				//	//	// �Ƿ�Ҫ֪ͨ�ӿ�
				//	//}

				//	//OnTrade(pField);
				//}
			}
		}
		else
		{
			//cout << "���ؽ��Ϊ��" << endl;
		}
	}
	else
	{
		//cout << "QueryOrder error:" << err_msg.error_no << err_msg.msg << endl;
		ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

		pField->ErrorID = m_err_msg.error_no;
		strcpy(pField->ErrorMsg, m_err_msg.msg);

		m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, true, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
	return 0;
}

void CTraderApi::ReqQryInstrument(const string& szInstrumentId, const string& szExchange)
{

}

void CTraderApi::OnPST16203PushData(PST16203PushData pEtxPushData)
{
	OrderIDType orderId = { 0 };

	// ֻ�Ǵ�ӡ�ɽ�
	ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

	//pField->ErrorID = pRspInfo->error_no;
	sprintf(pField->ErrorMsg,"OnPST16203PushData %s",pEtxPushData->order_status_name);

	m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, true, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
}

void CTraderApi::OnPST16204PushData(PST16204PushData pEtxPushData)
{
	OrderIDType orderId = { 0 };

	// ֻ�Ǵ�ӡ�ɽ�
	ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

	//pField->ErrorID = pRspInfo->error_no;
	sprintf(pField->ErrorMsg, "OnPST16204PushData %s", pEtxPushData->order_status_name);

	m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, true, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
}