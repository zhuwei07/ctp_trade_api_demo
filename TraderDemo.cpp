// 一个简单的例子，介绍CQspFtdcTraderApi和CQspFtdcTraderSpi接口的使用。
// 本例将演示一个报单录入操作的过程
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
// 报单录入操作是否完成的标志
// 经纪公司代码
TThostFtdcBrokerIDType g_chBrokerID;
// 交易用户代码
TThostFtdcUserIDType g_chUserID;
// 投资者编号
TThostFtdcInvestorIDType g_chInvestorID;
//用户本地最大报单号
int g_UserOrderLocalID;
//const char *INI_FILE_NAME = "TraderDemo.ini";
//bool bLogin = false;

class  CThread
{
public:
	/**构造函数
	*/
	CThread()
	{
		m_hThread = (THREAD_HANDLE)0;
		m_IDThread = 0;
	}

	/**析构函数
	*/
	virtual ~CThread() {}

	/**创建一个线程
	* @return true:创建成功 false:创建失败
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

		//20140613 xuzh 如果设置为0，join部分就无法join了，导致资源无法释放
		//pThread->m_hThread = (THREAD_HANDLE)0;

		return NULL;
	}

	/**虚函数，子类可做一些实例化工作
	* @return true:创建成功 false:创建失败
	*/
	virtual bool InitInstance()
	{
		return true;
	}

	/**虚函数，子类清楚实例
	*/
	virtual void ExitInstance() {}

	/**线程开始运行，纯虚函数，子类必须继承实现
	*/
	virtual void Run() = 0;

private:
	THREAD_HANDLE m_hThread;	/**< 线程句柄 */
	DWORD m_IDThread;

};

class CSimpleHandler : public CThostFtdcTraderSpi
{
public:
	// 构造函数，需要一个有效的指向CQspFtdcMduserApi实例的指针
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
		
		// 发出登陆请求
		m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
	}

	int ReqAuth()
	{
		strncpy(g_chBrokerID, "9999", sizeof(g_chBrokerID));
		strncpy(g_chUserID, "190340", sizeof(g_chUserID));
		strncpy(g_chInvestorID, "190340", sizeof(g_chInvestorID));
		//先认证
		CThostFtdcReqAuthenticateField reqmsg;
		memset(&reqmsg, 0, sizeof(CThostFtdcReqAuthenticateField));
		strncpy(reqmsg.BrokerID, g_chBrokerID, sizeof(reqmsg.BrokerID));
		strncpy(reqmsg.UserID, g_chUserID, sizeof(reqmsg.UserID));
		strncpy(reqmsg.AppID, "simnow_client_test", sizeof(reqmsg.AppID));
		strncpy(reqmsg.AuthCode, "0000000000000000", sizeof(reqmsg.AuthCode));

		int iRet = m_pUserApi->ReqAuthenticate(&reqmsg, 0);
		return iRet;
	}

	// 当客户端与量投科技建立起通信连接，客户端需要进行登录
	virtual void OnFrontConnected()
	{
		printf("OnFrontConnected\n");
		ReqAuth();
	}


	// 当客户端与量投科技通信连接断开时，该方法被调用
	virtual void OnFrontDisconnected(int nReason)
	{
		// 当发生这个情况后，API会自动重新连接，客户端可不做处理

		printf("OnFrontDisconnected, Reason=[%d].\n", nReason);
	}

	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (NULL == pRspAuthenticateField || NULL == pRspInfo)
		{
			return;
		}
		if (pRspInfo->ErrorID == 0)
		{
			//认证成功，开始登录
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
		//经纪公司代码
		strcpy(ord.BrokerID, g_chBrokerID);
		//交易所
		strcpy(ord.ExchangeID, "CFFEX");
		//投资者代码
		strcpy(ord.InvestorID, "guofu000009");
		// 用户代码
		strcpy(ord.UserID, g_chUserID);
		// 合约代码
		strcpy(ord.InstrumentID, "T1906");
		//本地报单号
		sprintf(ord.OrderRef, "%012d", ++g_UserOrderLocalID);
		//sprintf_s(ord.UserOrderLocalID,"%012d", 91000001);

		//报单类型
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		// 买卖方向
		//ord.Direction = THOST_FTDC_D_Buy;
		ord.Direction = THOST_FTDC_D_Sell;
		// 开平标志
		ord.CombOffsetFlag[0] = '0';
		// 投机套保标志
		ord.CombHedgeFlag[0] = '1';
		ord.LimitPrice = 98;
		// 数量
		ord.VolumeTotalOriginal = 1;
		// 有效期类型
		//strcpy(&(ord.TimeCondition),"3" );
		ord.TimeCondition = THOST_FTDC_TC_GFD;
		//成交量类型
		ord.VolumeCondition = THOST_FTDC_VC_AV;
		//强平原因
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;

		// 自动挂起标志
		ord.IsAutoSuspend = 0;

		int ret = m_pUserApi->ReqOrderInsert(&ord, 0);
		return ret;
	}

	int SendOrderAction()
	{
		CThostFtdcInputOrderActionField Quote;
		memset(&Quote, 0, sizeof(Quote));
		//经纪公司代码
		strcpy(Quote.BrokerID, g_chBrokerID);
		//交易所
		//strcpy(Quote.ExchangeID, "CZCE");
		strcpy(Quote.ExchangeID, "CFFEX");
		//strcpy(Quote.ExchangeID, "DCE");
		//投资者代码
		strcpy(Quote.InvestorID, g_chInvestorID);
		// 用户代码
		strcpy(Quote.UserID, g_chUserID);
		//报单操作标志
		Quote.ActionFlag = THOST_FTDC_AF_Delete;
		//本地报单号
		sprintf(Quote.OrderRef, "%012d", g_UserOrderLocalID++);

		//系统报单编号
		sprintf(Quote.OrderSysID, "");
		int ret = m_pUserApi->ReqOrderAction(&Quote, 0);
		return ret;
	}

	// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
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
			// 端登失败，客户端需进行错误处理
			printf("Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			//			exit(-1);
			//SLEEP(1000); //过1s再登录
			//Login();
		}

		//用户最大本地报单号
		g_UserOrderLocalID = atoi(pRspUserLogin->MaxOrderRef) + 2;

		printf("MaxOrderRef = [%s]\n", pRspUserLogin->MaxOrderRef);
		printf("g_UserOrderLocalID = [%d]\n", g_UserOrderLocalID);

		//bLogin = true;
		//ReqQryPosition();
	}

	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		// 输出报单录入结果
		if (NULL == pRspInfo || NULL == pInputOrder)
		{
			return;
		}
		printf("OnRspOrderInsert OrderRef=[%s], LimitPrice=[%f], ErrorCode=[%d], ErrorMsg=[%s]\n",
			pInputOrder->OrderRef, pInputOrder->LimitPrice, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}

	///报单回报
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

	//合约交易状态通知
	void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
	{
		/*printf("OnRtnInstrumentStatus: 合约交易状态通知 ExchangeID=[%s] InstrumentID=[%s], InstrumentStatus=[%c]\n",
			pInstrumentStatus->ExchangeID, pInstrumentStatus->InstrumentID, pInstrumentStatus->InstrumentStatus);*/
	}


private:
	// 指向CQspFtdcMduserApi实例的指针
	CThostFtdcTraderApi *m_pUserApi;
};


int main()
{
	// 产生一个CQspFtdcTraderApi实例
	CThostFtdcTraderApi *pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();

	// 产生一个事件处理的实例
	CSimpleHandler sh(pUserApi);

	// 注册一事件处理的实例	
	pUserApi->RegisterSpi(&sh);

	// 订阅私有流
	pUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);

	// 订阅公共流
	pUserApi->SubscribePublicTopic(THOST_TERT_RESTART);

	// 设置量投科技服务的地址，可以注册多个地址备用
	pUserApi->RegisterFront("tcp://180.168.146.187:10130"); // 7*24
	//pUserApi->RegisterFront("tcp://180.168.146.187:10202");
	pUserApi->Init();

	pUserApi->Join();
	// 释放API实例
//	pUserApi->Release();
	SLEEP(1000);
	return 0;
}
