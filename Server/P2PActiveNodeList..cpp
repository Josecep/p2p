#include "stdafx.h"
#include "P2PActiveNodeList.h"
#include "json\json.h"

CP2PActiveNodeList P2POnlineList;


CP2PActiveNodeList::CP2PActiveNodeList()
{
	::InitializeCriticalSection(&cs);
}


CP2PActiveNodeList::~CP2PActiveNodeList()
{
	::DeleteCriticalSection(&cs);
}


void CP2PActiveNodeList::push_back(string ip, unsigned short port)
{
	P2PAtiveNode * pNew = new P2PAtiveNode;
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

void CP2PActiveNodeList::remove(string ip, unsigned short port)
{
	::EnterCriticalSection(&cs);
	list<P2PAtiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		P2PAtiveNode * pNew = *it;
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

void CP2PActiveNodeList::sendalive(SOCKET s, string name)
{
	SOCKADDR_IN dst = { 0 };
	char sendbuf[2048] = { 0 };

	::EnterCriticalSection(&cs);
	list<P2PAtiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		P2PAtiveNode * pNew = *it;
		if (pNew)
		{
			dst.sin_family = AF_INET;
			dst.sin_addr.S_un.S_addr = inet_addr(pNew->ip.c_str());
			dst.sin_port = htons(pNew->port);

			Json::Value root;

			root["name"] = name;

			root["type"] = "p2palive";

			std::string text = root.toStyledString();

			sprintf(sendbuf, "%s", text.c_str());

			sendto(s, sendbuf, strlen(sendbuf), 0, (sockaddr *)&dst, sizeof(SOCKADDR_IN));
		}
	}
	::LeaveCriticalSection(&cs);

}


void CP2PActiveNodeList::FlushList(DWORD dwMs)
{
	::EnterCriticalSection(&cs);
	list<P2PAtiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		P2PAtiveNode * pNew = *it;
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

void CP2PActiveNodeList::Clear()
{
	::EnterCriticalSection(&cs);

	list<P2PAtiveNode *>::iterator it = nodelist.begin();

	for (; it != nodelist.end(); it++)
	{
		P2PAtiveNode * pTemp = *it;

		if (pTemp)
			delete pTemp;

	}

	nodelist.clear();

	::LeaveCriticalSection(&cs);
}

//名字必须唯一
void CP2PActiveNodeList::Flush(string name, string ip, unsigned short port)
{
	bool bFind = false;

	P2PAtiveNode * pTempNode = NULL;

	::EnterCriticalSection(&cs);

	list<P2PAtiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		P2PAtiveNode * pNew = *it;
		if (pNew)
		{
			if (pNew->nane == name)
			{
				bFind = true;
				pTempNode = pNew;
				break;
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
		P2PAtiveNode * pNew = new P2PAtiveNode;
		if (pNew)
		{
			pNew->bOnLine = true;
			pNew->ip = ip;
			pNew->llTime = LIVE_TIME;
			pNew->nane = name;
			pNew->port = port;
			nodelist.push_back(pNew);
		}
	}

	::LeaveCriticalSection(&cs);
}


//根据名字查找ip，port
bool CP2PActiveNodeList::find(string name, string & ip, unsigned short & port)
{
	bool bFind = false;
	::EnterCriticalSection(&cs);
	list<P2PAtiveNode *>::iterator it = nodelist.begin();
	for (; it != nodelist.end(); it++)
	{
		P2PAtiveNode * pNew = *it;
		if (pNew && pNew->bOnLine)
		{
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

