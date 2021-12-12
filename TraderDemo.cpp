// һ���򵥵����ӣ�����CQspFtdcTraderApi��CQspFtdcTraderSpi�ӿڵ�ʹ�á�
// ��������ʾһ������¼������Ĺ���
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <cstdlib>
#include "ThostFtdcTraderApi.h"
#include <iostream>  
#include <fstream>


#ifdef WIN32
#define WINDOWS
#endif
using namespace std;

#ifdef WINDOWS
#include <windows.h>
typedef HANDLE THREAD_HANDLE;
#define SLEEP(ms) Sleep(ms)
#else
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
typedef pthread_t THREAD_HANDLE;
#define SLEEP(ms) sleep((ms)/1000)
typedef unsigned int	DWORD;
#endif
// ����¼������Ƿ���ɵı�־
// ���͹�˾����
TThostFtdcBrokerIDType g_chBrokerID;
// �����û�����
TThostFtdcUserIDType g_chUserID;
// Ͷ���߱��
TThostFtdcInvestorIDType g_chInvestorID;
//�û�������󱨵���
int g_UserOrderLocalID;
//const char *INI_FILE_NAME = "TraderDemo.ini";
//bool bLogin = false;

class  CThread
{
public:
	/**���캯��
	*/
	CThread()
	{
		m_hThread = (THREAD_HANDLE)0;
		m_IDThread = 0;
	}

	/**��������
	*/
	virtual ~CThread() {}

	/**����һ���߳�
	* @return true:�����ɹ� false:����ʧ��
	*/
	virtual bool Create()
	{
		if (m_hThread != (THREAD_HANDLE)0)
		{
			return true;
		}
		bool ret = true;
#ifdef WIN32
		m_hThread = ::CreateThread(NULL, 0, _ThreadEntry, this, 0, &m_IDThread);
		if (m_hThread == NULL)
		{
			ret = false;
		}
#else
		ret = (::pthread_create(&m_hThread, NULL, &_ThreadEntry, this) == 0);
#endif
		return ret;
	}

	void ExitThread()
	{
#ifdef WIN32
		::ExitThread(0);
#endif
	}

private:
#ifdef WIN32
	static DWORD WINAPI _ThreadEntry(LPVOID pParam)
#else
	static void *_ThreadEntry(void *pParam)
#endif
	{
		CThread *pThread = (CThread *)pParam;
		if (pThread->InitInstance())
		{
			pThread->Run();
		}

		pThread->ExitInstance();

		//20140613 xuzh �������Ϊ0��join���־��޷�join�ˣ�������Դ�޷��ͷ�
		//pThread->m_hThread = (THREAD_HANDLE)0;

		return NULL;
	}

	/**�麯�����������һЩʵ��������
	* @return true:�����ɹ� false:����ʧ��
	*/
	virtual bool InitInstance()
	{
		return true;
	}

	/**�麯�����������ʵ��
	*/
	virtual void ExitInstance() {}

	/**�߳̿�ʼ���У����麯�����������̳�ʵ��
	*/
	virtual void Run() = 0;

private:
	THREAD_HANDLE m_hThread;	/**< �߳̾�� */
	DWORD m_IDThread;

};

class CSimpleHandler : public CThostFtdcTraderSpi
{
public:
	// ���캯������Ҫһ����Ч��ָ��CQspFtdcMduserApiʵ����ָ��
	CSimpleHandler(CThostFtdcTraderApi *pUserApi) :
		m_pUserApi(pUserApi)
	{
	}
	~CSimpleHandler() {}

	void CTP_simnow_Login()
	{
		CThostFtdcReqUserLoginField reqUserLogin;
		memset(&reqUserLogin, 0, sizeof(CThostFtdcReqUserLoginField));
		strncpy(reqUserLogin.UserProductInfo, "test", sizeof(reqUserLogin.UserProductInfo));
		strncpy(reqUserLogin.BrokerID, g_chBrokerID, sizeof(reqUserLogin.BrokerID));
		strncpy(reqUserLogin.UserID, g_chUserID, sizeof(reqUserLogin.UserID));
		strncpy(reqUserLogin.Password, "zhuwei@19930908", sizeof(reqUserLogin.Password));
		
		// ������½����
		m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
	}

	int ReqAuth()
	{
		strncpy(g_chBrokerID, "9999", sizeof(g_chBrokerID));
		strncpy(g_chUserID, "190340", sizeof(g_chUserID));
		strncpy(g_chInvestorID, "190340", sizeof(g_chInvestorID));
		//����֤
		CThostFtdcReqAuthenticateField reqmsg;
		memset(&reqmsg, 0, sizeof(CThostFtdcReqAuthenticateField));
		strncpy(reqmsg.BrokerID, g_chBrokerID, sizeof(reqmsg.BrokerID));
		strncpy(reqmsg.UserID, g_chUserID, sizeof(reqmsg.UserID));
		strncpy(reqmsg.AppID, "simnow_client_test", sizeof(reqmsg.AppID));
		strncpy(reqmsg.AuthCode, "0000000000000000", sizeof(reqmsg.AuthCode));

		int iRet = m_pUserApi->ReqAuthenticate(&reqmsg, 0);
		return iRet;
	}

	// ���ͻ�������Ͷ�Ƽ�������ͨ�����ӣ��ͻ�����Ҫ���е�¼
	virtual void OnFrontConnected()
	{
		printf("OnFrontConnected\n");
		ReqAuth();
	}


	// ���ͻ�������Ͷ�Ƽ�ͨ�����ӶϿ�ʱ���÷���������
	virtual void OnFrontDisconnected(int nReason)
	{
		// ��������������API���Զ��������ӣ��ͻ��˿ɲ�������

		printf("OnFrontDisconnected, Reason=[%d].\n", nReason);
	}

	///�ͻ�����֤��Ӧ
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (NULL == pRspAuthenticateField || NULL == pRspInfo)
		{
			return;
		}
		if (pRspInfo->ErrorID == 0)
		{
			//��֤�ɹ�����ʼ��¼
			CTP_simnow_Login();
		}
		else
		{
			printf("ErrID=[%d], ErrMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	int SendOrder()
	{
		CThostFtdcInputOrderField ord;
		memset(&ord, 0, sizeof(CThostFtdcInputOrderField));
		//���͹�˾����
		strcpy(ord.BrokerID, g_chBrokerID);
		//������
		strcpy(ord.ExchangeID, "CFFEX");
		//Ͷ���ߴ���
		strcpy(ord.InvestorID, "guofu000009");
		// �û�����
		strcpy(ord.UserID, g_chUserID);
		// ��Լ����
		strcpy(ord.InstrumentID, "T1906");
		//���ر�����
		sprintf(ord.OrderRef, "%012d", ++g_UserOrderLocalID);
		//sprintf_s(ord.UserOrderLocalID,"%012d", 91000001);

		//��������
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		// ��������
		//ord.Direction = THOST_FTDC_D_Buy;
		ord.Direction = THOST_FTDC_D_Sell;
		// ��ƽ��־
		ord.CombOffsetFlag[0] = '0';
		// Ͷ���ױ���־
		ord.CombHedgeFlag[0] = '1';
		ord.LimitPrice = 98;
		// ����
		ord.VolumeTotalOriginal = 1;
		// ��Ч������
		//strcpy(&(ord.TimeCondition),"3" );
		ord.TimeCondition = THOST_FTDC_TC_GFD;
		//�ɽ�������
		ord.VolumeCondition = THOST_FTDC_VC_AV;
		//ǿƽԭ��
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;

		// �Զ������־
		ord.IsAutoSuspend = 0;

		int ret = m_pUserApi->ReqOrderInsert(&ord, 0);
		return ret;
	}

	int SendOrderAction()
	{
		CThostFtdcInputOrderActionField Quote;
		memset(&Quote, 0, sizeof(Quote));
		//���͹�˾����
		strcpy(Quote.BrokerID, g_chBrokerID);
		//������
		//strcpy(Quote.ExchangeID, "CZCE");
		strcpy(Quote.ExchangeID, "CFFEX");
		//strcpy(Quote.ExchangeID, "DCE");
		//Ͷ���ߴ���
		strcpy(Quote.InvestorID, g_chInvestorID);
		// �û�����
		strcpy(Quote.UserID, g_chUserID);
		//����������־
		Quote.ActionFlag = THOST_FTDC_AF_Delete;
		//���ر�����
		sprintf(Quote.OrderRef, "%012d", g_UserOrderLocalID++);

		//ϵͳ�������
		sprintf(Quote.OrderSysID, "");
		int ret = m_pUserApi->ReqOrderAction(&Quote, 0);
		return ret;
	}

	// ���ͻ��˷�����¼����֮�󣬸÷����ᱻ���ã�֪ͨ�ͻ��˵�¼�Ƿ�ɹ�
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspUserLogin:\n");
		if (NULL == pRspInfo || NULL == pRspUserLogin)
		{
			printf("NULL == pRspInfo || NULL == pRspUserLogin\n");
			return;
		}
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n",
			pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		printf("MaxOrderRef=[%s]\n", pRspUserLogin->MaxOrderRef);
		if (pRspInfo->ErrorID != 0)
		{
			// �˵�ʧ�ܣ��ͻ�������д�����
			printf("Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			//			exit(-1);
			//SLEEP(1000); //��1s�ٵ�¼
			//Login();
		}

		//�û���󱾵ر�����
		g_UserOrderLocalID = atoi(pRspUserLogin->MaxOrderRef) + 2;

		printf("MaxOrderRef = [%s]\n", pRspUserLogin->MaxOrderRef);
		printf("g_UserOrderLocalID = [%d]\n", g_UserOrderLocalID);

		//bLogin = true;
		//ReqQryPosition();
	}

	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		// �������¼����
		if (NULL == pRspInfo || NULL == pInputOrder)
		{
			return;
		}
		printf("OnRspOrderInsert OrderRef=[%s], LimitPrice=[%f], ErrorCode=[%d], ErrorMsg=[%s]\n",
			pInputOrder->OrderRef, pInputOrder->LimitPrice, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}

	///�����ر�
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder)
	{
		printf("OnRtnOrder->OrderSysID=[%s], OrderStatus=[%c], UserOrderLocalID=[%s]\n", pOrder->OrderSysID, pOrder->OrderStatus, pOrder->OrderRef);
	}

	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
	{
		printf("=================================================\n");
		printf("OnErrRtnOrderInsert ErrorID=[%d], ErrMsg=[%s].\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("=================================================\n");
	}

	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspOrderAction ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}

	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade)
	{
		printf("OnRtnTrade: OrderSysID=[%s], TradeID=[%s]\n", pTrade->OrderSysID, pTrade->TradeID);
	}

	//��Լ����״̬֪ͨ
	void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
	{
		/*printf("OnRtnInstrumentStatus: ��Լ����״̬֪ͨ ExchangeID=[%s] InstrumentID=[%s], InstrumentStatus=[%c]\n",
			pInstrumentStatus->ExchangeID, pInstrumentStatus->InstrumentID, pInstrumentStatus->InstrumentStatus);*/
	}


private:
	// ָ��CQspFtdcMduserApiʵ����ָ��
	CThostFtdcTraderApi *m_pUserApi;
};


int main()
{
	// ����һ��CQspFtdcTraderApiʵ��
	CThostFtdcTraderApi *pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();

	// ����һ���¼������ʵ��
	CSimpleHandler sh(pUserApi);

	// ע��һ�¼������ʵ��	
	pUserApi->RegisterSpi(&sh);

	// ����˽����
	pUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);

	// ���Ĺ�����
	pUserApi->SubscribePublicTopic(THOST_TERT_RESTART);

	// ������Ͷ�Ƽ�����ĵ�ַ������ע������ַ����
	pUserApi->RegisterFront("tcp://180.168.146.187:10130"); // 7*24
	//pUserApi->RegisterFront("tcp://180.168.146.187:10202");
	pUserApi->Init();

	pUserApi->Join();
	// �ͷ�APIʵ��
//	pUserApi->Release();
	SLEEP(1000);
	return 0;
}
