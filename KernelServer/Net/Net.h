#ifndef _NET_H
#define _NET_H

#pragma comment(lib,"ws2_32.lib")

class Net
{
	public:
		Net(void){}
		virtual ~Net(void){}
	public:
		virtual bool InitNet() = 0;
		virtual void UnInitNet() = 0;
		virtual bool SendData(int socketClient,const char* pszBuffer,int nSendLen) = 0;
};
#endif
