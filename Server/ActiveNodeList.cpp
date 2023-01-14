#include "stdafx.h"
#include "ActiveNodeList.h"
#include "json\json.h"

CActiveNodeList OnlineList;


CActiveNodeList::CActiveNodeList()
{
	::InitializeCriticalSection(&cs);
}


CActiveNodeList::~CActiveNodeList()
{
	::DeleteCriticalSection(&cs);
}


void CActiveNodeList::push_back(string ip, unsigned short port)
{
	ActiveNode * pNew = new ActiveNode;
	if (!pNew)
		return;
	pNew->ip = ip;
	pNew->port = port;
	pNew->llTime = LIVE_TIME;
	pNew->bOnLine = true;
	::EnterCriticalSection(&cs);
	nodelist.push_back(pNew);
	::LeaveCriticalSection(&cs);
}

void CActiveNodeList::remove(string ip, unsigned short port)
{
	::EnterCriticalSection(&cs);
	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew)
		{
			if ((pNew->ip == ip) && (pNew->port == port))
			{
				delete pNew;
				nodelist.erase(it);
				break;
			}
		}
	}
	::LeaveCriticalSection(&cs);

}

void CActiveNodeList::sendalive(SOCKET s,string name)
{
	SOCKADDR_IN dst = { 0 };
	char sendbuf[2048] = { 0 };

	::EnterCriticalSection(&cs);
	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew)
		{
			dst.sin_family = AF_INET;
			dst.sin_addr.S_un.S_addr = inet_addr(pNew->ip.c_str());
			dst.sin_port = htons(pNew->port);

			Json::Value root;

			root["name"] = name;

			root["type"] = "alive";

			std::string text = root.toStyledString();

			sprintf(sendbuf,"%s",text.c_str());

			sendto(s, sendbuf, strlen(sendbuf), 0, (sockaddr *)&dst, sizeof(SOCKADDR_IN));
		}
	}
	::LeaveCriticalSection(&cs);

}

bool CActiveNodeList::sendlist2(SOCKET s, SOCKADDR_IN  dst, CServerDlg * pDlg)
{
	char sendbuf[2048] = { 0 };
	string ip;
	ip = inet_ntoa(dst.sin_addr);
	int nPort = 0;
	nPort = ntohs(dst.sin_port);
	::EnterCriticalSection(&cs);
	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew && pNew->bOnLine)
		{

			if ((pNew->ip == ip) && (pNew->port == nPort))
				continue;

			Json::Value root;

			root["type"] = "list client ok";

			root["ip"] = pNew->ip;

			root["port"] = pNew->port;

			root["name"] = pNew->nane;

			std::string text = root.toStyledString();

			sprintf(sendbuf, "%s", text.c_str());

			int nVal = sendto(s, sendbuf, strlen(sendbuf), 0, (sockaddr *)&dst, sizeof(SOCKADDR_IN));

			if (nVal > 0)
			{
				if (pDlg)
					pDlg->PrintSend(s, sendbuf, dst);
			}
		}
	}
	::LeaveCriticalSection(&cs);

	return true;
}

void CActiveNodeList::FlushList(DWORD dwMs)
{
	::EnterCriticalSection(&cs);
	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew && pNew->bOnLine)
		{
			pNew->llTime -= dwMs;
			if (pNew->llTime <= 0)
			{
				pNew->bOnLine = false;
			}
		}
	}
	::LeaveCriticalSection(&cs);
	return;
}

//名字必须唯一
void CActiveNodeList::Flush(string ip, unsigned short port, string name,bool bServer)
{
	bool bFind = false;

	ActiveNode * pTempNode = NULL;

	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew)
		{
			if (bServer)
			{
				if (pNew->nane == name)
				{
					bFind = true;
					pTempNode = pNew;
					break;
				}
			}
			else
			{
				if ((pNew->ip == ip) && (pNew->port == port))
				{
					bFind = true;
					pTempNode = pNew;
					break;
				}
			}
		
		}
	}


	if (bFind && pTempNode)
	{
		pTempNode->ip = ip;
		pTempNode->port = port;
		pTempNode->bOnLine = true;
		pTempNode->llTime = LIVE_TIME;
	}
	else
	{
		ActiveNode * pNew = new ActiveNode;
		if (pNew)
		{
			pNew->ip = ip;
			pNew->port = port;
			pNew->llTime = LIVE_TIME;
			pNew->bOnLine = true;
			pNew->nane = name;
			nodelist.push_back(pNew);
		}
	}
	::LeaveCriticalSection(&cs);
}


bool CActiveNodeList::find(string ip, unsigned short port)
{
	bool bFind = false;
	::EnterCriticalSection(&cs);
	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew && pNew->bOnLine)
		{
			bool b1 = (pNew->ip == ip);
			bool b2 = (pNew->port == port);

			if ((pNew->ip == ip) && (pNew->port == port))
			{
				bFind = true;
				break;
			}	
		}
	}
	::LeaveCriticalSection(&cs);
	return bFind;
}

//根据名字查找ip，port
bool CActiveNodeList::find(string name, string & ip, unsigned short & port)
{
	bool bFind = false;
	::EnterCriticalSection(&cs);
	list<ActiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		ActiveNode * pNew = *it;
		if (pNew && pNew->bOnLine)
		{
			bool b1 = (pNew->ip == ip);
			bool b2 = (pNew->port == port);

			if (pNew->nane == name) 
			{
				bFind = true;
				ip = pNew->ip;
				port = pNew->port;
				break;
			}
		}
	}
	::LeaveCriticalSection(&cs);
	return bFind;
}

