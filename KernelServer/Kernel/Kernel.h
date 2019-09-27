#ifndef _KERNEL_H
#define _KERNEL_H

#include "Net.h"

class Kernel
{
	protected:
		Net* m_pNet;
	public:
		Kernel(){m_pNet = 0;}
		virtual ~Kernel(){}
	public:
		Net* GetNet()
		{
			return m_pNet;
		}
	public:
		virtual bool InitKernel() = 0;
		virtual void UnInitKernel() = 0;
		virtual void DealData(int socketClient, char* szBuffer, int nBufferLen) = 0;
};

#endif
