#pragma once

#include <list>
#include <string>
#include "ServerDlg.h"
#include <winsock2.h>
using namespace std;

#define LIVE_TIME 300000

typedef struct _tagP2PAtiveNode
{
	string ip;
	unsigned short port;
	bool bOnLine;
	//以毫秒为单位，客户端每间隔100ms发送一个心跳包给服务器，服务器如果5分钟没有收到任何客户端的信息，则认为结点下线，所以这里的值：5*60*1000 = 300000
	long long llTime;
	string nane;
	_tagP2PAtiveNode()
	{
		port = 0;
		llTime = LIVE_TIME;
		bOnLine = true;
	}
}P2PAtiveNode;


class CP2PActiveNodeList
{
public:
	CP2PActiveNodeList();
	~CP2PActiveNodeList();

	void push_back(string ip, unsigned short port);
	void remove(string ip, unsigned short port);
	
	//刷新列表，以毫秒为单位
	void FlushList(DWORD dwMs);


	//根据名字查找ip，port
	bool find(string name, string & ip, unsigned short & port);

	//发送心跳包,name是发送者的名字
	void sendalive(SOCKET s, string name);

	void Flush(string name, string ip, unsigned short port);

	//清空
	void Clear();

	friend class CServerDlg;

private:
	list<P2PAtiveNode *> nodelist;
	CRITICAL_SECTION cs;

};

extern CP2PActiveNodeList P2POnlineList;

