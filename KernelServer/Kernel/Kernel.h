#include "INet.h"

class IKernel
{
	protected:
		INet* m_pNet;
	public:
		IKernel(){m_pNet = 0;}
		virtual ~IKernel(){}
	public:
		INet* GetNet()
		{
			return m_pNet;
		}
	public:
		virtual bool InitKernel() = 0;
		virtual void UnInitKernel() = 0;
		virtual void DealData(int socketClient, char* szBuffer, int nBufferLen) = 0;
};
