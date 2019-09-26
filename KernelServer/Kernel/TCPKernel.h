#ifndef TCPKERNEL_H
#define TCPKERNEL_H
#include "IKernel.h"
#include "MyThreadPool.h"
#include "CMySql.h"
#include "Protocol.h"
#include <map>
using namespace std;


#define PROTOCOL_MAP_BEGIN() STRU_PROTOCOL_MAP protocol_map_entry[] = {
#define PROTOCOL(ProtocolType,ProtocolProc) {ProtocolType,ProtocolProc},
#define PROTOCOL_MAP_END()  {0,0}};

//  C++成员 函数指针
class CTCPKernel;
typedef void (CTCPKernel::*PFUN)(int,char*,int);
struct STRU_PROTOCOL_MAP
{
	PackType packtype;  //  协议
	PFUN pfn;          //  处理协议的函数指针
};

//   -----------------主播观众列表-------------------------
#define STREAM_PAGE_SIZE  4148
#define STREAM_COUNT      3000

struct AuthorNode
{
	long long llRoomID;   //  房间号
	char szAuthorName[NAME_SIZE];                  //  主播名
	char szRoomName[NAME_SIZE];                     //  房间名
	//HANDLE hSemaphorVedio;                          //  推送视频流的信号量
	char szVedioContent[STREAM_PAGE_SIZE*STREAM_COUNT];  //  视频流的缓存
	int nVedioOffset;                                   //  偏移量
	int socketAuthor;                               //  主播socket 
};
struct AudienceNode
{
	char szAudienceName[NAME_SIZE]; // 观众名
	int socketAudience;       //  观众socket 
	int nVedioOffset;          //  偏移量
	//HANDLE hEventQuitAuthor;    //  退出这个主播
};

class CTCPKernel : public IKernel
{
	private:
		//CMyThreadPool threadpool;  //  线程池
		//CMySql sql;                //  数据库
	public:
		list<AuthorNode*> m_lstAuthorList;   //  装所有的主播
		list<AudienceNode*> m_lstAudienceList;   //  观众列表
		map<AuthorNode*,list<AudienceNode*>> m_mpAuthorToAudience;
	public:
		CTCPKernel();
		virtual ~CTCPKernel();
	public:
		virtual bool InitKernel();
		virtual void UnInitKernel();
		virtual void DealData(int socketClient,char* szBuffer,int nBufferLen);
	public:
		void RegisterRQ(int socketClient,char* szBuffer,int nBufferLen);
		void LoginRQ(int socketClient,char* szBuffer,int nBufferLen);
		void GetRoomInfoRQ(int socketClient,char* szBuffer,int nBufferLen);
		void SetRoomInfoRQ(int socketClient,char* szBuffer,int nBufferLen);
		void BeginVedioRQ(int socketClient,char* szBuffer,int nBufferLen);
		void GetAuthorListRQ(int socketClient,char* szBuffer,int nBufferLen);
		void VedioStreamInfo(int socketClient,char* szBuffer,int nBufferLen);
		void VedioStream(int socketClient,char* szBuffer,int nBufferLen);
		void SelectAuthorRQ(int socketClient,char* szBuffer,int nBufferLen);
};
#endif
