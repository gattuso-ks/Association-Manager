#include "TCPNet.h"
#include <process.h>
#include <iostream>
using namespace std;

Kernel* TCPNet::m_pKernel = 0;
bool TCPNet::m_bThreadQuitFlag = true;//  线程是否继续执行

TCPNet::TCPNet(Kernel* pKernel)
{
	m_pKernel = pKernel;
	socketServer = 0;
	m_hAcccptThread = 0;
}


TCPNet::~TCPNet(void)
{
	this->UnInitNet();
}

bool TCPNet::InitNet()
{
	socketServer = socket(AF_INET,SOCK_STREAM,0);
	if(socketServer < 0)
	{
		return false;
	}

	sockaddr_in addrServer;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = //htons(PROT);
	if(bind(socketServer,(const sockaddr*)&addrServer,sizeof(addrServer)) != 0)
	{
		return false;
	}

	if(listen(socketServer,SOMAXCONN) < 0) //  SOMAXCONN 监听个数
	{
		return false;
	}

	//创建线程监听 TODO
	m_hAcccptThread = (HANDLE)_beginthreadex(0,0,&TCPNet::AcceptThreadProc,this,0,0);
	return true;
}
void TCPNet::UnInitNet()
{
	//  线程退出
	m_bThreadQuitFlag = false;
	//  结束m_hAcccptThread 线程
	if(m_hAcccptThread != 0)
	{
		if(::WaitForSingleObject(m_hAcccptThread,10) == WAIT_TIMEOUT)
			::TerminateThread(m_hAcccptThread,0);
		::CloseHandle(m_hAcccptThread);
		m_hAcccptThread = 0;
	}
	//  关闭所有的收发数据的线程
	list<HANDLE>::iterator ite = m_lstRecvSendThread.begin();
	while(ite != m_lstRecvSendThread.end())
	{
		if(::WaitForSingleObject((*ite),10) == WAIT_TIMEOUT)
			::TerminateThread((*ite),0);
		::CloseHandle((*ite));
		(*ite) = 0;

		++ite;
	}

	//  关闭socket
	if(socketServer != 0)
	{
		::closesocket(socketServer);
		socketServer = 0;
	}
	//  卸载库
	::WSACleanup();
}
bool TCPNet::SendData(SOCKET socketClient,const char* pszBuffer,int nSendLen)
{
	if(socketClient == 0 || pszBuffer == 0 || nSendLen <= 0)
		return false;
	//  发送4个字节包大小
	if(send(socketClient,(const char*)&nSendLen,4,0) <= 0)
		return false;
	//  发送内容
	if(send(socketClient,(const char*)pszBuffer,nSendLen,0) <= 0)
		return false;
	return true;
}


unsigned int __stdcall TCPNet::AcceptThreadProc( void * pVoid)
{
	TCPNet* pThis = (TCPNet*)pVoid;

	while(pThis->m_bThreadQuitFlag)
	{
		sockaddr_in addr;
		int nLen = sizeof(addr);
		//  不停的接受客户端连接
		SOCKET socketClient = ::accept(pThis->socketServer,(sockaddr*)&addr,&nLen);
		if(socketClient != INVALID_SOCKET)
		{
			//  创建和 socketClient 客户端收发数据的线程
			cout << "IP:" << inet_ntoa(addr.sin_addr) << "已经连接！！！" << endl;
			HANDLE hThread = (HANDLE)_beginthreadex(0,0,&TCPNet::RecvThreadProc,(void*)socketClient,0,0);
			if(hThread != 0)
				pThis->m_lstRecvSendThread.push_back(hThread);
		}
	}
	return 0;
}

unsigned int __stdcall TCPNet::RecvThreadProc( void * pVoid)
{
	SOCKET socketClient = (SOCKET)pVoid;
	//  接收数据
	while(TCPNet::m_bThreadQuitFlag)
	{
		//----------------------接收包的大小
		int nPackSize = 0;
		if(::recv(socketClient,(char*)&nPackSize,4,0) <= 0)
		{
			//  接收失败
			// 判断客户端有没有关闭
			int nCodeError = ::WSAGetLastError();
			if(WSAECONNRESET == nCodeError)   // 客户端已经关闭
				cout << "客户端已经关闭" << endl;
			cout << "code error" << nCodeError << endl;
			return 0;
		}
		//----------------------收包的内容------------------------
		char* pszRecvBuffer = new char[nPackSize];
		::ZeroMemory(pszRecvBuffer,nPackSize);
		int nOffset = 0;
		while(1)
		{	
			int nRecvLen = ::recv(socketClient,(char*)pszRecvBuffer+nOffset,nPackSize-nOffset,0);
			if(nRecvLen <= 0)
			{
				// 判断客户端有没有关闭
				int nCodeError = ::WSAGetLastError();
				if(WSAECONNRESET == nCodeError)   // 客户端已经关闭
					cout << "客户端已经关闭" << endl;
				cout << "code error" << nCodeError << endl;
				return 0;
			}
			nOffset += nRecvLen; 
			if(nOffset >= nPackSize)
				break;
		}
		//-----------------------------交给Kernel处理-------------------------
		TCPNet::m_pKernel->DealData(socketClient,pszRecvBuffer,nPackSize);
		//-----------------------------交给Kernel处理-------------------------
		delete pszRecvBuffer;
		pszRecvBuffer = 0;
	}
	return 0;
}
