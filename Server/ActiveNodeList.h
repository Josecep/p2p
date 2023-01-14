#pragma once
#include <list>
#include <string>
#include "ServerDlg.h"
#include <winsock2.h>
using namespace std;

#define LIVE_TIME 300000

typedef struct _tagActiveNode
{
	string ip;
	unsigned short port;
	bool bOnLine;
	//以毫秒为单位，客户端每间隔100ms发送一个心跳包给服务器，服务器如果5分钟没有收到任何客户端的信息，则认为结点下线，所以这里的值：5*60*1000 = 300000
	long long llTime;
	string nane;
	_tagActiveNode()
	{
		port = 0;
		llTime = LIVE_TIME;
		bOnLine = true;
	}
}ActiveNode;


class CActiveNodeList
{
public:
	CActiveNodeList();
	~CActiveNodeList();

	void push_back(string ip, unsigned short port);
	void remove(string ip, unsigned short port);
	//发送在线列表到某个客户端
	bool sendlist2(SOCKET s, SOCKADDR_IN  dst, CServerDlg * pDlg);
	//刷新列表，以毫秒为单位
	void FlushList(DWORD dwMs);
	//刷新结点
	void Flush(string ip, unsigned short port, string name, bool bServer=true);

	bool find(string ip, unsigned short port);

	//根据名字查找ip，port
	bool find(string name, string & ip, unsigned short & port);

	//发送心跳包
	void sendalive(SOCKET s, string name);

friend class CServerDlg;

private:
	list<ActiveNode *> nodelist;
	CRITICAL_SECTION cs;

};

extern CActiveNodeList OnlineList;

