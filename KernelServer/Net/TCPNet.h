#ifndef _TCPNET_H
#define _TCPNET_H

#include <winsock2.h>
#include <windows.h>
#include <list>
#include "Protocol.h"
#include "Net.h"
#include "Kernel.h"
using namespace std;
#pragma comment(lib,"ws2_32.lib")

class TCPNet : public Net
{
	private:
		static Kernel* m_pKernel;        //  处理
		int socketServer;				//  监听socket
		//HANDLE m_hAcccptThread;			 //  接受连接的线程
		static bool m_bThreadQuitFlag;			//  线程退出
		list<HANDLE> m_lstRecvSendThread;  //  每个客户端 都定义有一个线程负责收发数据 
	public:
		TCPNet(IKernel* pKernel);
		~TCPNet(void);
	public:
		bool InitNet();
		void UnInitNet();
		bool SendData(int socketClient,const char* pszBuffer,int nSendLen);
	public:  
		static unsigned int __stdcall AcceptThreadProc( void * );     //  接受连接的线程函数
		static unsigned int __stdcall RecvThreadProc( void * );      //  接收数据的线程
};

#endif
