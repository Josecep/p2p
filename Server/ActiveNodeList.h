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
	//�Ժ���Ϊ��λ���ͻ���ÿ���100ms����һ���������������������������5����û���յ��κοͻ��˵���Ϣ������Ϊ������ߣ����������ֵ��5*60*1000 = 300000
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
	//���������б�ĳ���ͻ���
	bool sendlist2(SOCKET s, SOCKADDR_IN  dst, CServerDlg * pDlg);
	//ˢ���б��Ժ���Ϊ��λ
	void FlushList(DWORD dwMs);
	//ˢ�½��
	void Flush(string ip, unsigned short port, string name, bool bServer=true);

	bool find(string ip, unsigned short port);

	//�������ֲ���ip��port
	bool find(string name, string & ip, unsigned short & port);

	//����������
	void sendalive(SOCKET s, string name);

friend class CServerDlg;

private:
	list<ActiveNode *> nodelist;
	CRITICAL_SECTION cs;

};

extern CActiveNodeList OnlineList;

