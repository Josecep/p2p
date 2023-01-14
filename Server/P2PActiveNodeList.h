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
	//�Ժ���Ϊ��λ���ͻ���ÿ���100ms����һ���������������������������5����û���յ��κοͻ��˵���Ϣ������Ϊ������ߣ����������ֵ��5*60*1000 = 300000
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
	
	//ˢ���б��Ժ���Ϊ��λ
	void FlushList(DWORD dwMs);


	//�������ֲ���ip��port
	bool find(string name, string & ip, unsigned short & port);

	//����������,name�Ƿ����ߵ�����
	void sendalive(SOCKET s, string name);

	void Flush(string name, string ip, unsigned short port);

	//���
	void Clear();

	friend class CServerDlg;

private:
	list<P2PAtiveNode *> nodelist;
	CRITICAL_SECTION cs;

};

extern CP2PActiveNodeList P2POnlineList;

