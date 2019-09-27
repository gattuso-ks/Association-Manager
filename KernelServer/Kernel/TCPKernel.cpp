#include "TCPKernel.h"
#include "TCPNet.h"
#include <iostream>
using namespace std;
#include "VedioTask.h"


PROTOCOL_MAP_BEGIN()

	PROTOCOL(PROTOCOL_REGISTER_RQ,&TCPKernel::RegisterRQ)
	PROTOCOL(PROTOCOL_LOGIN_RQ,&TCPKernel::LoginRQ)
	PROTOCOL(PROTOCOL_GET_ROOM_INFO_RQ,&TCPKernel::GetRoomInfoRQ)
	PROTOCOL(PROTOCOL_SET_ROOM_INFO_RQ,&TCPKernel::SetRoomInfoRQ)
	PROTOCOL(PROTOCOL_BEGIN_VEDIO_RQ,&TCPKernel::BeginVedioRQ)
	PROTOCOL(PROTOCOL_GET_AUTHOR_LIST_RQ,&TCPKernel::GetAuthorListRQ)
	PROTOCOL(PROTOCOL_VEDIO_STREAM_IMFO_RQ,&TCPKernel::VedioStreamInfo)
	PROTOCOL(PROTOCOL_VEDIO_STREAM_CONTENT_RQ,&TCPKernel::VedioStream)
	PROTOCOL(PROTOCOL_SELECT_AUTHOR_RQ,&TCPKernel::SelectAuthorRQ)

PROTOCOL_MAP_END()

TCPKernel::TCPKernel()
{
	m_pNet = new CTCPNet(this);   //  创建一个网络对象
}
TCPKernel::~TCPKernel()
{
	delete m_pNet;   //  删除网络
	m_pNet = 0;

	//  删除主播列表
	list<AuthorNode*>::iterator itrAuthor = m_lstAuthorList.begin();
	while(itrAuthor != m_lstAuthorList.end())
	{
		::CloseHandle((*itrAuthor)->hSemaphorVedio);
		(*itrAuthor)->hSemaphorVedio = 0;
		::closesocket((*itrAuthor)->socketAuthor);
		(*itrAuthor)->socketAuthor = 0;
		delete (*itrAuthor);
		itrAuthor = m_lstAuthorList.erase(itrAuthor);
	}
	//  删除观众列表
	list<AudienceNode*>::iterator iteAudience =  m_lstAudienceList.begin(); 
	while(iteAudience !=  m_lstAudienceList.end())
	{
		::WSACloseEvent((*iteAudience)->hEventQuitAuthor);
		(*iteAudience)->hEventQuitAuthor = 0;
		::closesocket((*iteAudience)->socketAudience);
		(*iteAudience)->socketAudience = 0;

		delete (*iteAudience);
		iteAudience = m_lstAudienceList.erase(iteAudience);
	}	
}

bool TCPKernel::InitKernel()
{
	//  网络的初始化
	if(m_pNet->InitNet() == false)
		return false;
	//  连接数据库
	if(sql.ConnectMySql("localhost","root","tangqiulin110","20190115lele") == false)
		return false;

	//  创建线程池
	if(threadpool.CreateThreadPool() == false)
		return false;

	return true;
}

void TCPKernel::UnInitKernel()
{
	//  断开数据库
	sql.DisConnect();
}

void TCPKernel::DealData(int socketClient,char* szBuffer,int nBufferLen)
{
	//  取出协议头
	PackType packtype = *szBuffer;
	//  遍历协议映射表
	STRU_PROTOCOL_MAP* pTemp = protocol_map_entry;
	while(1)
	{
		//  到结尾
		if(pTemp->packtype == 0 && pTemp->pfn == 0)
			return;

		// 匹配协议
		if(pTemp->packtype == packtype)
		{
			//  调用函数指针处理
			(this->*(pTemp->pfn))(socketClient,szBuffer,nBufferLen);
			return;
		}

		// 继续匹配
		pTemp++;
	}
}

void TCPKernel::RegisterRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_REGISTER_RQ* srRQ = (STRU_REGISTER_RQ*)szBuffer;
	STRU_REGISTER_RS srRS;
	srRS.packtype = PROTOCOL_REGISTER_RS;
	srRS.result = FAIL;
	char szSQL[100] = {0};   //  SQL语句

	//  判断用户类型
	if(srRQ->usertype == AUTHOR)
	{
		//  查询有没有
		sprintf_s(szSQL,"select a_name from author where a_name = %s",srRQ->szUserName);
		list<string> lst;
		if(sql.SelectMySql(szSQL,1,lst) == true && lst.size() == 1)
		{
			m_pNet->SendData(socketClient,(const char*)&srRS,sizeof(srRS));
			return ;
		}
		//  主播
		::ZeroMemory(szSQL,100);
		sprintf_s(szSQL,"insert into author value('%lld','%s','%s')",srRQ->llUserID,srRQ->szUserName,srRQ->szPassWord);
		if(sql.UpdateMySql(szSQL) == true)
		{
			srRS.result = SUCCESS;   //  完成
		}
		m_pNet->SendData(socketClient,(const char*)&srRS,sizeof(srRS));
	}
	else
	{
		//  观众
		sprintf_s(szSQL,"select au_name from audience where au_name = %s",srRQ->szUserName);
		list<string> lst;
		if(sql.SelectMySql(szSQL,1,lst) == true && lst.size() == 1)
		{
			m_pNet->SendData(socketClient,(const char*)&srRS,sizeof(srRS));
			return ;
		}
		//  主播
		::ZeroMemory(szSQL,100);
		sprintf_s(szSQL,"insert into audience value('%lld','%s','%s')",srRQ->llUserID,srRQ->szUserName,srRQ->szPassWord);
		if(sql.UpdateMySql(szSQL) == true)
		{
			srRS.result = SUCCESS;   //  完成
		}
		m_pNet->SendData(socketClient,(const char*)&srRS,sizeof(srRS));
	}
}

void TCPKernel::LoginRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_LOGIN_RQ* slRQ = (STRU_LOGIN_RQ*)szBuffer;
	STRU_LOGIN_RS slRS;
	slRS.packtype = PROTOCOL_LOGIN_RS;
	slRS.result = FAIL;
	char szSQL[100] = {0};

	if(slRQ->usertype == AUTHOR)
	{
		sprintf_s(szSQL,"select a_password from author where a_name = '%s'",slRQ->szUserName);
		// 查找密码
		list<string> lst;
		if(sql.SelectMySql(szSQL,1,lst) == true && lst.size() > 0)
		{
			// 判断密码
			string strPassWord = lst.front();
			lst.pop_back();
			if(strcmp(strPassWord.c_str(),slRQ->szPassWord) == 0)
			{
				slRS.result = SUCCESS;
			}
		}
	}
	else
	{
		sprintf_s(szSQL,"select au_password from audience where au_name = '%s'",slRQ->szUserName);
		// 查找密码
		list<string> lst;
		if(sql.SelectMySql(szSQL,1,lst) == true && lst.size() > 0)
		{
			// 判断密码
			string strPassWord = lst.front();
			lst.pop_back();
			if(strcmp(strPassWord.c_str(),slRQ->szPassWord) == 0)
			{
				//---------------把观众加到列表中-----------------
				AudienceNode* pAudience = new AudienceNode;
				pAudience->hEventQuitAuthor = ::WSACreateEvent();
				pAudience->nVedioOffset = 0;
				pAudience->socketAudience = socketClient;
				strcpy_s(pAudience->szAudienceName,NAME_SIZE,slRQ->szUserName);
				m_lstAudienceList.push_back(pAudience);

				slRS.result = SUCCESS;
			}
		}
	}

	//  回复
	m_pNet->SendData(socketClient,(const char*)&slRS,sizeof(slRS));
}

void TCPKernel::GetRoomInfoRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_GET_ROOM_INFO_RQ* sgriRQ = (STRU_GET_ROOM_INFO_RQ*)szBuffer;
	STRU_GET_ROOM_INFO_RS sgriRS;
	::ZeroMemory(&sgriRS,sizeof(sgriRS));
	sgriRS.packtype = PROTOCOL_GET_ROOM_INFO_RS;

	//  获取房间ID 和 名字
	char szSQL[100] = {0};
	sprintf_s(szSQL,"select r_id,r_name from room where a_name = '%s'",sgriRQ->szAuthorName);
	list<string> lst;
	if(sql.SelectMySql(szSQL,2,lst) == true && lst.size() > 0)
	{
		string strID = lst.front();
		lst.pop_front();
		string strRoomName = lst.front();
		lst.pop_front();
		//  把数据装到 包
		sgriRS.llRoomID = _atoi64(strID.c_str());
		strcpy_s(sgriRS.szRoomName,NAME_SIZE,strRoomName.c_str());
	}
	//  发送
	m_pNet->SendData(socketClient,(const char*)&sgriRS,sizeof(sgriRS));
}

void TCPKernel::SetRoomInfoRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_SET_ROOM_INFO_RQ* sgriRQ = (STRU_SET_ROOM_INFO_RQ*)szBuffer;
	STRU_SET_ROOM_INFO_RS sgriRS;
	sgriRS.packtype = PROTOCOL_SET_ROOM_INFO_RS;
	sgriRS.result = FAIL;

	//  修改数据库中的房间信息
	char szSQL[100] = {0};
	sprintf_s(szSQL,"update room set r_name = '%s' where a_name = '%s'",sgriRQ->szRoomName,sgriRQ->szAuthorName);

	if(sql.UpdateMySql(szSQL) == true)
		sgriRS.result = SUCCESS;

	//  回复
	m_pNet->SendData(socketClient,(const char*)&sgriRS,sizeof(sgriRS));
}

void TCPKernel::BeginVedioRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_BEGIN_VEDIO_RQ* sbviRQ = (STRU_BEGIN_VEDIO_RQ*)szBuffer;
	STRU_BEGIN_VEDIO_RS sbviRS;
	sbviRS.packtype = PROTOCOL_BEGIN_VEDIO_RS;
	sbviRS.result = FAIL;

	char szSQL[100] = {0};
	sprintf_s(szSQL,"select r_id,r_name from room where a_name = '%s'",sbviRQ->szAuthorName);
	list<string> lst;
	if(sql.SelectMySql(szSQL,2,lst) == true && lst.size() > 0)
	{
		string strID = lst.front();
		lst.pop_front();
		string strRoomName = lst.front();
		lst.pop_front();

		//  加入到主播列表中
		AuthorNode* pAuthor = new AuthorNode;
		::ZeroMemory(pAuthor,sizeof(AuthorNode));
		pAuthor->hSemaphorVedio = ::CreateSemaphore(0,0,1000,0);
		pAuthor->llRoomID = _atoi64(strID.c_str());
		pAuthor->socketAuthor = socketClient;
		strcpy_s(pAuthor->szAuthorName,NAME_SIZE,sbviRQ->szAuthorName);
		strcpy_s(pAuthor->szRoomName,NAME_SIZE,strRoomName.c_str());
		// 添加到链表中
		m_lstAuthorList.push_back(pAuthor);

		sbviRS.result = SUCCESS;
	}
	//  回复
	m_pNet->SendData(socketClient,(const char*)&sbviRS,sizeof(sbviRQ));
}

void TCPKernel::GetAuthorListRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_GET_AUTHOR_LIST_RQ* sgalRQ = (STRU_GET_AUTHOR_LIST_RQ*)szBuffer;
	STRU_GET_AUTHOR_LIST_RS sgqlRS;
	::ZeroMemory(&sgqlRS,sizeof(sgqlRS));
	sgqlRS.packtype = PROTOCOL_GET_AUTHOR_LIST_RS;

	// 遍历主播列表
	list<AuthorNode*>::iterator iteAuthor = m_lstAuthorList.begin();
	while(1)
	{
		//  主播没有的
		if(iteAuthor == m_lstAuthorList.end())
		{
			m_pNet->SendData(socketClient,(const char*)&sgqlRS,sizeof(sgqlRS));
			break;
		}
		//  判断回复包 中的主播列表是否装满了
		if(sgqlRS.nListLen == AUTHOR_LIST_COUNT)
		{
			m_pNet->SendData(socketClient,(const char*)&sgqlRS,sizeof(sgqlRS));
			//  清空回复包
			::ZeroMemory(&sgqlRS,sizeof(sgqlRS));
			sgqlRS.packtype = PROTOCOL_GET_AUTHOR_LIST_RS;
			continue;
		}
		//  装到主播列表中
		sgqlRS.authorlist[sgqlRS.nListLen].llRoomID = (*iteAuthor)->llRoomID;
		strcpy_s(sgqlRS.authorlist[sgqlRS.nListLen].szAuthorName,NAME_SIZE,(*iteAuthor)->szAuthorName);
		strcpy_s(sgqlRS.authorlist[sgqlRS.nListLen].szRoomName,NAME_SIZE,(*iteAuthor)->szRoomName);

		++(sgqlRS.nListLen);
		++iteAuthor;	
	}
}

void TCPKernel::VedioStreamInfo(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_VEDIO_STREAM_RQ* sbsRQ = (STRU_VEDIO_STREAM_RQ*)szBuffer;
	//  在 主播和观众的映射表中 查找  对应的主播
	AuthorNode* pAuthor = 0;
	list<AuthorNode*>::iterator iteAuthor = m_lstAuthorList.begin();
	while(iteAuthor != m_lstAuthorList.end())
	{
		if(strcmp((*iteAuthor)->szAuthorName,sbsRQ->szAuthorName) == 0)
		{
			pAuthor = (*iteAuthor);
			break;
		}
		++iteAuthor;
	}
	//  判断有没有观众
	if(pAuthor != 0 && m_mpAuthorToAudience[pAuthor].size() > 0)
	{
		//  把视频流内容存进去
		::memcpy(pAuthor->szVedioContent+pAuthor->nVedioOffset*STREAM_PAGE_SIZE,sbsRQ,STREAM_PAGE_SIZE);
		pAuthor->nVedioOffset = (pAuthor->nVedioOffset+1)%STREAM_COUNT;
		//  释放信号量
		::ReleaseSemaphore(pAuthor->hSemaphorVedio,m_mpAuthorToAudience[pAuthor].size(),0);
	}
}

void TCPKernel::VedioStream(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_VEDIO_STREAM_RQ* sbsRQ = (STRU_VEDIO_STREAM_RQ*)szBuffer;

	//  在 主播和观众的映射表中 查找  对应的主播
	AuthorNode* pAuthor = 0;
	list<AuthorNode*>::iterator iteAuthor = m_lstAuthorList.begin();
	while(iteAuthor != m_lstAuthorList.end())
	{
		if(strcmp((*iteAuthor)->szAuthorName,sbsRQ->szAuthorName) == 0)
		{
			pAuthor = (*iteAuthor);
			break;
		}
		++iteAuthor;
	}
	//  判断有没有观众
	if(pAuthor != 0 && m_mpAuthorToAudience[pAuthor].size() > 0)
	{
		//  把视频流内容存进去
		::memcpy(pAuthor->szVedioContent+pAuthor->nVedioOffset*STREAM_PAGE_SIZE,sbsRQ,STREAM_PAGE_SIZE);
		pAuthor->nVedioOffset = (pAuthor->nVedioOffset+1)%STREAM_COUNT;
		//  释放信号量
		::ReleaseSemaphore(pAuthor->hSemaphorVedio,m_mpAuthorToAudience[pAuthor].size(),0);
	}
}


void TCPKernel::SelectAuthorRQ(int socketClient,char* szBuffer,int nBufferLen)
{
	STRU_SELECT_AUTHOR_RQ* ssaRQ = (STRU_SELECT_AUTHOR_RQ*)szBuffer;
	STRU_SELECT_AUTHOR_RS ssaRS;
	ssaRS.packtype = PROTOCOL_SELECT_AUTHOR_RS;

	//  查找主播
	AuthorNode* pAuthor = 0;
	list<AuthorNode*>::iterator iteAuthor = m_lstAuthorList.begin();
	while(iteAuthor != m_lstAuthorList.end())
	{
		if((*iteAuthor)->llRoomID == ssaRQ->llRoomID)
		{
			pAuthor = (*iteAuthor);
			break;
		}
		++iteAuthor;
	}

	//  查找观众
	AudienceNode* pAudience = 0;
	list<AudienceNode*>::iterator iteAudience = m_lstAudienceList.begin();
	while(iteAudience != m_lstAudienceList.end())
	{
		if(strcmp((*iteAudience)->szAudienceName,ssaRQ->szAudienceName) == 0)
		{
			pAudience = (*iteAudience);
			break;
		}
		++iteAudience;
	}

	//  建立映射关系
	if(pAuthor != 0 && pAudience != 0)
	{
		m_mpAuthorToAudience[pAuthor].push_back(pAudience);
		pAudience->nVedioOffset = pAuthor->nVedioOffset;
		//  线程池分配一个线程等待直播视频流
		CVedioTask* pTask = new CVedioTask(this,pAuthor,pAudience);
		threadpool.PushTask(pTask);
		ssaRS.result = SUCCESS;
	}
	//  回复
	m_pNet->SendData(socketClient,(const char*)&ssaRS,sizeof(ssaRS));
}
