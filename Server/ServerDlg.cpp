
// ServerDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ActiveNodeList.h"
#include "P2PActiveNodeList.h"
#include <regex>
#include "json/json.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_SOCK_READ WM_USER + 1
#define WM_FLUSH_SERVER_LIST WM_USER + 100
#define WM_FLUSH_CLIENT_LIST WM_USER + 200
#define NAT_OK_TIMER 100
#define P2P_TIMER 200

volatile BOOL bStop = TRUE;
HANDLE hOnLineThread = NULL;
volatile BOOL bP2PStop = TRUE;
HANDLE hP2POnLineThread = NULL;

DWORD WINAPI OnLineThreadProc(_In_ LPVOID lpParameter);
DWORD WINAPI P2PThreadProc(_In_ LPVOID lpParameter);


HANDLE hEvent;


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CServerDlg �Ի���



CServerDlg::CServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
	, m_strContent(_T(""))
	, m_nListenPort(0)
	, nPort(0)
	, m_strSend(_T(""))
	, m_bServer(TRUE)
	, m_nSvrPort(0)
	, m_strName(_T(""))
	, m_nWorkMode(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	hEvent = ::CreateEvent(0, TRUE, TRUE, 0);

}

CServerDlg::~CServerDlg()
{
	if (m_socket)
		closesocket(m_socket);

	std::list<Node *>::iterator it = lists.begin();

	for (; it != lists.end(); it++)
	{
		Node * pTemp = *it;
		if (pTemp)
			delete pTemp;
	}

	lists.clear();

	if (hEvent)
		CloseHandle(hEvent);
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_IP_PORT, m_AddrList);
	DDX_Text(pDX, IDC_EDIT_CONTENT, m_strContent);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nListenPort);
	DDX_Control(pDX, IDC_IPADDRESS_DST, m_ctrIp);
	DDX_Text(pDX, IDC_EDIT_DST_PORT, nPort);
	DDX_Text(pDX, IDC_EDIT_CONTENT_SEND, m_strSend);

	DDX_Control(pDX, IDC_LIST_ONLINE, m_ActiveNodeList);
	DDX_Control(pDX, IDC_LIST_PEER_ONLINE, m_PeerList);
	DDX_Control(pDX, IDC_IPADDRESS_SVR_IP, m_svrIp);
	DDX_Text(pDX, IDC_EDIT_SVR_PORT, m_nSvrPort);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Control(pDX, IDC_BUTTON_CONN_PEER, m_btnServer);
	DDX_Radio(pDX, IDC_RADIO_SERVER, m_nWorkMode);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CServerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CServerDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_START, &CServerDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CServerDlg::OnBnClickedButtonStop)
	ON_MESSAGE(WM_SOCK_READ,OnSockRead)
	ON_MESSAGE(WM_FLUSH_SERVER_LIST,OnFlushServerList)
	ON_MESSAGE(WM_FLUSH_CLIENT_LIST, OnFlushClientList)
	ON_WM_TIMER()

	ON_NOTIFY(NM_CLICK, IDC_LIST_IP_PORT, &CServerDlg::OnNMClickListIpPort)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CServerDlg::OnBnClickedButtonSend)

	ON_BN_CLICKED(IDC_BUTTON_GET_ONLINE, &CServerDlg::OnBnClickedButtonGetOnline)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_PEER_ONLINE, &CServerDlg::OnNMRClickListPeerOnline)
	ON_BN_CLICKED(IDC_BUTTON_CONN_PEER, &CServerDlg::OnBnClickedButtonConnPeer)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CServerDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_RADIO_SERVER, &CServerDlg::OnBnClickedRadioServer)
	ON_BN_CLICKED(IDC_RADIO_CLIENT, &CServerDlg::OnBnClickedRadioClient)
	ON_BN_CLICKED(IDC_BUTTON_P2P_SEND, &CServerDlg::OnBnClickedButtonP2pSend)
END_MESSAGE_MAP()

void CServerDlg::SetPeerLink(string name)
{
	CString strName;

	CStringA strTempA;

	CString strTemp;

	strTempA = name.c_str();

	strTemp = strTempA;

	int nCount = m_PeerList.GetItemCount();

	for (int i = 0; i < nCount; i++)
	{
		strName = m_PeerList.GetItemText(i, 0);
		if (strName == strTemp)
		{
			m_PeerList.SetItemText(i, 3, _T("������"));
			break;
		}
	}

}

void CServerDlg::OnTimer(UINT nIDEvent)
{
	CStringA strTempA;
	CString strTemp;
	CString strName;


	if (nIDEvent == NAT_OK_TIMER)
	{
		std::list<CNode>::iterator it = timerlist.begin();

		while (it != timerlist.end())
		{
			if ((*it).nCount <= 0)
			{
				
				strTempA = it->name.c_str();
				strTemp = strTempA;
				int nCount = m_PeerList.GetItemCount();
				bool bFind = false;
				int nIndex = -1;
				for (int i = 0; i < nCount; i++)
				{
					strName = m_PeerList.GetItemText(i, 0);
					if (strName == strTemp)
					{
						bFind = true;
						nIndex = i;
						break;
					}
				}
				if (bFind)
				{
					m_PeerList.SetItemText(nIndex, 3, _T("����ʧ��!"));
				}

				timerlist.erase(it);

				it = timerlist.begin();

			}
			else
			{

				Json::Value root;

				root["type"] = "p2p test";
		
				strTempA = m_strName;

				root["name"] = strTempA.GetBuffer(strTempA.GetLength());

				string strCmd = root.toStyledString();

				for (int i = 0; i < 100; i++)
				{
					sendto(m_socket, strCmd.c_str(), strCmd.length(), 0, (sockaddr *)&it->addr, sizeof(it->addr));
				}
				(*it).nCount--;
				it++;
			}
		} 
	
	}
	else if (nIDEvent == P2P_TIMER)
	{
		std::list<CNode>::iterator it = p2ptimer.begin();

		while (it != p2ptimer.end())
		{
			if ((*it).nCount <= 0)
			{
				p2ptimer.erase(it);

				it = p2ptimer.begin();

			}
			else
			{
				Json::Value root;
				
				root["type"] = "p2p test ack";
			
				strTempA = m_strName;
				root["name"] = strTempA.GetBuffer(strTempA.GetLength());

				string strCmd = root.toStyledString();

				for (int i = 0; i < 100; i++)
				{
					sendto(m_socket, strCmd.c_str(), strCmd.length(), 0, (sockaddr *)&it->addr, sizeof(it->addr));
				}
				(*it).nCount--;
				it++;
			}
		}

	}
}

void CServerDlg::FlushPeerAddr(string name, string ip, unsigned int port)
{
	int nCount = m_PeerList.GetItemCount();
	bool bFind = false;
	int nIndex = -1;
	CString strName;
	CStringA strNameA;
	strNameA = name.c_str();
	strName = strNameA;
	CString strTemp;
	CStringA strTempA;
	for (int i = 0; i < nCount; i++)
	{
		strTemp = m_PeerList.GetItemText(i, 0);
		if (strName == strTemp)
		{
			bFind = true;
			nIndex = i;
			break;
		}
	}
	if (bFind)
	{
		strTempA = ip.c_str();
		strTemp = strTempA;
		m_PeerList.SetItemText(nIndex, 1, strTemp);
		strTempA.Format("%d",port);
		strTemp = strTempA;
		m_PeerList.SetItemText(nIndex, 2, strTemp);
	}
	return;
}




LRESULT CServerDlg::OnFlushServerList(WPARAM w, LPARAM l)
{
	int nIndex = 0;

	CStringA strTempA;
	CString strTemp;

	m_ActiveNodeList.DeleteAllItems();

	::EnterCriticalSection(&OnlineList.cs);

	list<ActiveNode* >::iterator it = OnlineList.nodelist.begin();

	for (; it != OnlineList.nodelist.end(); it++)
	{
		ActiveNode* pNew = *it;
		if (pNew)
		{
			strTempA = pNew->nane.c_str();
			strTemp = strTempA;
			nIndex = m_ActiveNodeList.InsertItem(m_ActiveNodeList.GetItemCount(), strTemp);

			strTempA = pNew->ip.c_str();
			strTemp = strTempA;

			m_ActiveNodeList.SetItemText(nIndex, 1, strTemp);
		
			strTempA.Format("%d",pNew->port);
			strTemp = strTempA;
			m_ActiveNodeList.SetItemText(nIndex, 2, strTemp);


			if (pNew->bOnLine)
			{
				m_ActiveNodeList.SetItemText(nIndex, 3, _T("����"));
			}
			else
			{
				m_ActiveNodeList.SetItemText(nIndex, 3, _T("����"));
			}
	
		}
		
	}

	::LeaveCriticalSection(&OnlineList.cs);

	return 0;
}


LRESULT CServerDlg::OnFlushClientList(WPARAM w, LPARAM l)
{
	int nIndex = 0;

	CStringA strTempA;
	CString strTemp;

	m_ActiveNodeList.DeleteAllItems();

	::EnterCriticalSection(&OnlineList.cs);

	list<ActiveNode* >::iterator it = OnlineList.nodelist.begin();

	for (; it != OnlineList.nodelist.end(); it++)
	{
		ActiveNode* pNew = *it;
		if (pNew)
		{
			nIndex = m_ActiveNodeList.InsertItem(m_ActiveNodeList.GetItemCount(), _T("������û������"));
			strTempA = pNew->ip.c_str();
			strTemp = strTempA;

			m_ActiveNodeList.SetItemText(nIndex, 1, strTemp);
				
			strTempA.Format("%d", pNew->port);
			strTemp = strTempA;
			m_ActiveNodeList.SetItemText(nIndex, 2, strTemp);


			if (pNew->bOnLine)
			{
				m_ActiveNodeList.SetItemText(nIndex, 3, _T("����"));
			}
			else
			{
				m_ActiveNodeList.SetItemText(nIndex, 3, _T("����"));
			}

		}

	}

	::LeaveCriticalSection(&OnlineList.cs);

	return 0;
}

//��ʾ����
bool CServerDlg::PrintfRecv(SOCKET s, char * buf, int nBufLen, SOCKADDR_IN & client)
{
	SOCKADDR_IN self = { 0 };
	int nSelfLen = sizeof(SOCKADDR_IN);
	CStringA strTempA;
	CString strTemp;
	Node * pNew = new Node();
	if (!pNew)
		return false;
	memcpy(pNew->data, buf, nBufLen);
	strcpy(pNew->srcip, inet_ntoa(client.sin_addr));
	pNew->srcport = ntohs(client.sin_port);
	getsockname(s, (SOCKADDR*)&self, &nSelfLen);
	strcpy(pNew->dstip, inet_ntoa(self.sin_addr));
	pNew->dstport = ntohs(self.sin_port);
	pNew->nDir = RECV;

	strTempA = pNew->srcip;
	strTemp = strTempA;

	int nTempIndex = m_AddrList.InsertItem(m_AddrList.GetItemCount(), strTemp);

	strTempA.Format("%d", pNew->srcport);
	strTemp = strTempA;

	m_AddrList.SetItemText(nTempIndex, 1, strTemp);
	strTempA = pNew->dstip;
	strTemp = strTempA;
	m_AddrList.SetItemText(nTempIndex, 2, strTemp);
	strTempA.Format("%d", pNew->dstport);
	strTemp = strTempA;
	m_AddrList.SetItemText(nTempIndex, 3, strTemp);


	CTime time = CTime::GetCurrentTime();
	CString curTime;
	curTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"),
		time.GetYear(),
		time.GetMonth(),
		time.GetDay(),
		time.GetHour(),
		time.GetMinute(),
		time.GetSecond());


	m_AddrList.SetItemText(nTempIndex, 4, curTime);

	pNew->strTime = curTime;

	m_AddrList.SetItemText(nTempIndex, 5, _T("recv"));

	lists.push_back(pNew);

	return true;
}

//��ӡ���ͱ���

bool CServerDlg::PrintSend(SOCKET s, char * send, SOCKADDR_IN & client)
{
	int nTempIndex = 0;
	CStringA strTempA;
	CString strTemp;
	SOCKADDR_IN self = { 0 };
	int nSelfLen = sizeof(self);
	Node * pNew = new Node();
	if (!pNew)
		return false;

	memcpy(pNew->data, send, strlen(send));
	strcpy(pNew->dstip, inet_ntoa(client.sin_addr));
	pNew->dstport = ntohs(client.sin_port);
	getsockname(s, (SOCKADDR*)&self, &nSelfLen);
	strcpy(pNew->srcip, inet_ntoa(self.sin_addr));
	pNew->srcport = ntohs(self.sin_port);
	pNew->nDir = SEND;


	strTempA = pNew->srcip;

	strTemp = strTempA;

	nTempIndex = m_AddrList.InsertItem(m_AddrList.GetItemCount(), strTemp);

	strTempA.Format("%d", pNew->srcport);
	strTemp = strTempA;

	m_AddrList.SetItemText(nTempIndex, 1, strTemp);
	strTempA = pNew->dstip;
	strTemp = strTempA;
	m_AddrList.SetItemText(nTempIndex, 2, strTemp);
	strTempA.Format("%d", pNew->dstport);
	strTemp = strTempA;
	m_AddrList.SetItemText(nTempIndex, 3, strTemp);


	CTime time = CTime::GetCurrentTime();

	CString curTime;
	
	curTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"),
		time.GetYear(),
		time.GetMonth(),
		time.GetDay(),
		time.GetHour(),
		time.GetMinute(),
		time.GetSecond());


	m_AddrList.SetItemText(nTempIndex, 4, curTime);

	pNew->strTime = curTime;

	m_AddrList.SetItemText(nTempIndex, 5, _T("send"));

	lists.push_back(pNew);

	return true;
}


void CServerDlg::SetTimerDelay(std::list<CNode> & list, std::string name, int nCount)
{
	std::list<CNode>::iterator it = list.begin();
	for (; it != list.end(); it++)
	{
		if ((*it).name == name)
		{
			it->nCount = nCount;
			break;
		}
	}
	return;
}

void CServerDlg::DeleteTimer(std::list<CNode> & list, std::string name)
{
	std::list<CNode>::iterator it = list.begin();
	for (; it != list.end(); it++)
	{
		if ((*it).name == name)
		{
			list.erase(it);
			break;
		}
	}
	return;
}



void CServerDlg::FlushTimer(std::list<CNode> & list, std::string name, std::string ip, unsigned short port)
{
	SOCKADDR_IN DstAddr = { 0 };

	std::list<CNode>::iterator it = list.begin();
	for (; it != list.end(); it++)
	{
		if ((*it).name == name)
		{
			DstAddr.sin_family = AF_INET;
			DstAddr.sin_port = htons(port);
			DstAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

			it->addr = DstAddr;
			break;
		}
	}
	return;
}


LRESULT CServerDlg::OnSockRead(WPARAM w, LPARAM l)
{
	SOCKADDR_IN client = { 0 };
	SOCKADDR_IN self = { 0 };
	int nSelfLen = sizeof(self);

	int nClientAddrLen = sizeof(client);

	char buf[1024] = { 0 };

	char send[2048] = { 0 };

	SOCKET s = (SOCKET)w;

	int nTemp = 0;

	if (WSAGETSELECTERROR(l))
	{
		AfxMessageBox(_T("�������!"));
		return 0;
	}


	// ���������¼�
	switch (WSAGETSELECTEVENT(l))
	{
	case FD_READ:
	{
		nTemp = recvfrom(s, buf, 1024, 0, (SOCKADDR*)&client, &nClientAddrLen);

		if (nTemp > 0)
		{
			Json::Reader reader;
			Json::Value value;

			string data = buf;

			if (!reader.parse(data, value))
			{
				return 0;
			}

			string type = value["type"].asString();

			if (m_bServer)
			{
				//�ͻ��˵�¼
				if (type == "login")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);

					//ˢ���б�
					OnlineList.Flush(inet_ntoa(client.sin_addr), ntohs(client.sin_port), value["name"].asString());

					//������Ӧ��
					Json::Value root;
					root["type"] = "loginok";
					string temp = root.toStyledString().c_str();
					sprintf(send, "%s", temp.c_str());

					if (sendto(s, send, strlen(send), 0, (sockaddr *)&client, sizeof(client)) > 0)
					{
						//��ӡ���ͱ���
						PrintSend(s, send, client);
					}
				}
				//������
				//����ʾ�������ı���
				else if (type == "alive")
				{
					OnlineList.Flush(inet_ntoa(client.sin_addr), ntohs(client.sin_port), value["name"].asString());
					Json::Value root;
					root["type"] = "aliveack";
					sprintf(send, "%s", root.toStyledString().c_str());
					sendto(s, send, strlen(send), 0, (sockaddr *)&client, sizeof(client));
				}
				//��ȡ�ͻ����б�
				else if (type == "list client")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);

					OnlineList.sendlist2(s, client,this);
				}
				//��
				else if (type == "nat")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);

					string ip;

					unsigned short port = 0;
			
					string name = value["dstname"].asString();

					if (OnlineList.find(name,ip,port))
					{
						SOCKADDR_IN DstAddr = { 0 };
						int nLen = sizeof(DstAddr);

						DstAddr.sin_family = AF_INET;
						DstAddr.sin_port = htons(port);
						DstAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());


						string dstIp = inet_ntoa(client.sin_addr);
						int nDstPort = ntohs(client.sin_port);

						Json::Value root;
						root["type"] = "p2p";
						root["ip"] = dstIp;
						root["port"] = nDstPort;
						root["name"] = value["srcname"];
					
						string strCmd = root.toStyledString();

						int nRet = sendto(s, strCmd.c_str(), strCmd.length(), 0, (sockaddr *)&DstAddr, nLen);

						sprintf(send, "%s", strCmd.c_str());

						if (nRet > 0)
						{
							//��ӡ���ͱ���
							PrintSend(s, send, DstAddr);

							Json::Value root;
							root["type"] = "nat ok";
							root["ip"] = ip;
							root["port"] = port;
							root["name"] = name;

							string strCmd = root.toStyledString();

							int nRet = sendto(s, strCmd.c_str(), strCmd.length(), 0, (sockaddr *)&client, sizeof(client));

							if(nRet > 0)
							{
								sprintf(send, "%s", strCmd.c_str());
								//��ӡ���ͱ���
								PrintSend(s, send, DstAddr);
							}

						}
					}

				}
				else if (type == "data")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);
				}
			}
			else
			{
				if (type == "loginok")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);

					string ip = inet_ntoa(client.sin_addr);
					int port = ntohs(client.sin_port);
					OnlineList.Flush(ip, port, "", false);

				}
				//������
				//����ʾ�������ı���
				else if (type == "p2palive")
				{
					P2POnlineList.Flush(value["name"].asString(), inet_ntoa(client.sin_addr), ntohs(client.sin_port));
					Json::Value root;
					root["type"] = "p2paliveack";
					CStringA strTempA;
					strTempA = m_strName;
					string name = strTempA.GetBuffer(strTempA.GetLength());
					root["name"] = name;
					sprintf(send, "%s", root.toStyledString().c_str());
					sendto(s, send, strlen(send), 0, (sockaddr *)&client, sizeof(client));
				}
				else if (type == "p2paliveack")
				{
					P2POnlineList.Flush(value["name"].asString(), inet_ntoa(client.sin_addr), ntohs(client.sin_port));
				}
				else if (type == "aliveack")
				{
					//����������ʾ
					string ip = inet_ntoa(client.sin_addr);
					int port = ntohs(client.sin_port);
					OnlineList.Flush(ip, port, "", false);
				}
				else if (type == "p2p")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);

					string ip = value["ip"].asString();
					unsigned short nPort = value["port"].asInt();

					SOCKADDR_IN addr = { 0 };
					addr.sin_family = AF_INET;
					addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
					addr.sin_port = htons(nPort);

					CNode n;
					n.addr = addr;
					n.nCount = 300;
					p2ptimer.push_back(n);

				}
				else if (type == "p2p test")
				{
					std::string name;
					name = value["name"].asString();
					DeleteTimer(p2ptimer, name);
					FlushTimer(p2ptimer,name, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		
				}
				else if (type == "p2p test ack")
				{
					string name;
					name = value["name"].asString();
					DeleteTimer(timerlist, name);
					SetPeerLink(name);
					P2POnlineList.Flush(value["name"].asString(), inet_ntoa(client.sin_addr), ntohs(client.sin_port));
					//��ʱ���ͣ�Ŀ����Ϊ�˶Է�ȷ���յ�p2p test

				}
				else if (type == "nat ok")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);
					string ip = value["ip"].asString();
					int nPort = value["port"].asInt();
					string name = value["name"].asString();

					//ˢ�£���ֹip,port�仯
					FlushPeerAddr(name, ip, nPort);

					SOCKADDR_IN addr = { 0 };
					addr.sin_family = AF_INET;
					addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
					addr.sin_port = htons(nPort);

					CNode n;
					n.addr = addr;
					n.nCount = 300;
					n.name = name;
		
					timerlist.push_back(n);

				}
				else if (type == "list client ok")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);
					string name = value["name"].asString();
					string ip = value["ip"].asString();
					int port = value["port"].asInt();

					CStringA strTempA;
					strTempA = name.c_str();
					CString strTemp;
					strTemp = strTempA;

					int nIndex = m_PeerList.InsertItem(m_PeerList.GetItemCount(), strTemp);

					strTempA = ip.c_str();
					strTemp = strTempA;
					m_PeerList.SetItemText(nIndex, 1, strTemp);


					strTempA.Format("%d", port);
					strTemp = strTempA;
					m_PeerList.SetItemText(nIndex, 2, strTemp);

				}
				else if (type == "data")
				{
					//��ʾ���յ���Ϣ����
					PrintfRecv(s, buf, nTemp, client);
				}
			}
		}

		break;
	}
	}
	return 0;
}


// CServerDlg ��Ϣ�������

BOOL CServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	m_AddrList.SetExtendedStyle(m_AddrList.GetExtendedStyle()
		| LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);


	m_AddrList.InsertColumn(0, _T("ԴIP"), LVCFMT_LEFT, 80);
	m_AddrList.InsertColumn(1, _T("Դ�˿�"), LVCFMT_LEFT,80);
	m_AddrList.InsertColumn(2, _T("Ŀ��IP"), LVCFMT_LEFT, 80);
	m_AddrList.InsertColumn(3, _T("Ŀ�Ķ˿�"), LVCFMT_LEFT, 80);
	m_AddrList.InsertColumn(4, _T("ʱ��"), LVCFMT_LEFT, 160);
	m_AddrList.InsertColumn(5, _T("����"), LVCFMT_LEFT, 120);




	m_ActiveNodeList.SetExtendedStyle(m_ActiveNodeList.GetExtendedStyle()
		| LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_ActiveNodeList.InsertColumn(0, _T("����"), LVCFMT_LEFT, 160);
	m_ActiveNodeList.InsertColumn(1, _T("���IP"), LVCFMT_LEFT, 160);
	m_ActiveNodeList.InsertColumn(2, _T("���˿�"), LVCFMT_LEFT, 120);
	m_ActiveNodeList.InsertColumn(3, _T("����"), LVCFMT_LEFT, 160);
	

	m_PeerList.SetExtendedStyle(m_PeerList.GetExtendedStyle()
		| LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	
	m_PeerList.InsertColumn(0, _T("����"), LVCFMT_LEFT, 160);
	m_PeerList.InsertColumn(1, _T("���IP"), LVCFMT_LEFT, 160);
	m_PeerList.InsertColumn(2, _T("���˿�"), LVCFMT_LEFT, 120);
	m_PeerList.InsertColumn(3, _T("�Ե�����"), LVCFMT_LEFT, 160);




	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	
//	��MFC�¿�������̨
	//AllocConsole();
	//freopen("CONOUT$", "a+", stdout);

	

	

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CServerDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
//	CDialogEx::OnOK();
}


void CServerDlg::OnBnClickedCancel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (m_bServer)
	{
		bStop = TRUE;
		WaitForSingleObject(hOnLineThread, INFINITE);
	}

	CDialogEx::OnCancel();
}


void CServerDlg::OnBnClickedButtonStart()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	UpdateData(TRUE);

	//m_AddrList.DeleteAllItems();

	if (m_socket)
		closesocket(m_socket);

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


	SOCKADDR_IN ServerAddr = { 0 };
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(m_nListenPort);
	ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;


	if (bind(m_socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{
		AfxMessageBox(_T("�󶨼���ʧ��!"));
		return;
	}

	WSAAsyncSelect(m_socket, m_hWnd, WM_SOCK_READ, FD_READ);

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);

	bStop = FALSE;
	bP2PStop = FALSE;

	if (m_bServer)
	{
		hOnLineThread = ::CreateThread(0, 0, OnLineThreadProc, this, 0, 0);
		
		SetDlgItemText(IDC_STATIC_NODELIST_CAP, _T("�ͻ������߽���б�"));
		SetWindowText(_T("������"));
	}
	else
	{
		hP2POnLineThread = ::CreateThread(0, 0, P2PThreadProc, this, 0, 0);
		hOnLineThread = ::CreateThread(0, 0, OnLineThreadProc, this, 0, 0);
		SetDlgItemText(IDC_STATIC_NODELIST_CAP, _T("���������߽���б�"));
		SetWindowText(_T("�ͻ���"));
		SetTimer(P2P_TIMER, 100, 0);
		SetTimer(NAT_OK_TIMER, 100, 0);
	}
	
	GetDlgItem(IDC_RADIO_SERVER)->EnableWindow(FALSE);
	GetDlgItem(IDC_RADIO_CLIENT)->EnableWindow(FALSE);
}


void CServerDlg::OnBnClickedButtonStop()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (m_socket)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	if (m_bServer)
	{
		bStop = TRUE;
		WaitForSingleObject(hOnLineThread, INFINITE);
	}
	else
	{
		bStop = TRUE;
		WaitForSingleObject(hOnLineThread, INFINITE);
		bP2PStop = TRUE;
		WaitForSingleObject(hP2POnLineThread, INFINITE);

		KillTimer(P2P_TIMER);
		KillTimer(NAT_OK_TIMER);
	}

	GetDlgItem(IDC_RADIO_SERVER)->EnableWindow(TRUE);
	GetDlgItem(IDC_RADIO_CLIENT)->EnableWindow(TRUE);
}


CString CServerDlg::GetData(CString strIp, CString strPort, CString DstIp, CString DstPort, CString strTime,int nIndex)
{
	CStringA strTempSrcIpA;
	CString strTempSrcIp;

	CStringA strTempSrcPortA;
	CString strTempSrcPort;

	CStringA strTempDstIpA;
	CString strTempDstIp;

	CStringA strTempDstPortA;
	CString strTempDstPort;

	CString strTimeTemp;

	std::list<Node *>::iterator it = lists.begin();

	int i = 0;

	for (; it != lists.end(); it++,i++)
	{
		Node * pTemp = *it;
		if (pTemp)
		{
			strTempSrcIpA.Format("%s", pTemp->srcip);
			strTempSrcIp = strTempSrcIpA;

			strTempSrcPortA.Format("%d", pTemp->srcport);
			strTempSrcPort = strTempSrcPortA;

			strTempDstIpA.Format("%s", pTemp->dstip);
			strTempDstIp = strTempDstIpA;

			strTempDstPortA.Format("%d", pTemp->dstport);
			strTempDstPort = strTempDstPortA;

			strTimeTemp = pTemp->strTime;

			if ((strTempSrcIp == strIp) && (strPort == strTempSrcPort) && (DstIp == strTempDstIp) && (DstPort == strTempDstPort) && (strTime == strTimeTemp) && (i == nIndex))
			{
				CStringA strTempA;
				strTempA = pTemp->data;
				CString strTemp;
				strTemp = strTempA;
				return strTemp;
			}
		}
	}

	return _T("");
}


void CServerDlg::OnNMClickListIpPort(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	int nIndex = m_AddrList.GetSelectionMark();

	CString SrcIp;
	CString SrcPort;
	CString DstIp;
	CString DstPort;
	CString strTime;

	CString strInfo;

	if (nIndex != -1)
	{
		SrcIp = m_AddrList.GetItemText(nIndex, 0);
		SrcPort = m_AddrList.GetItemText(nIndex, 1);
		DstIp = m_AddrList.GetItemText(nIndex, 2);
		DstPort = m_AddrList.GetItemText(nIndex, 3);
		strTime = m_AddrList.GetItemText(nIndex, 4);

		strInfo = GetData(SrcIp, SrcPort, DstIp, DstPort, strTime, nIndex);

		GetDlgItem(IDC_EDIT_CONTENT)->SetWindowText(strInfo);
	}
}


void CServerDlg::OnBnClickedButtonSend()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	DWORD dwIp;

	char send[2048] = { 0 };

	Node * pNew = NULL;

	SOCKADDR_IN DstAddr = { 0 };
	int nLen = sizeof(DstAddr);

	SOCKADDR_IN self = { 0 };

	int nSelfLen = sizeof(self);

	UpdateData(TRUE);

	m_strName.Trim();

	if (!m_bServer && m_strName.IsEmpty())
	{
		AfxMessageBox(_T("�������û���!"));
		return;
	}


	Json::Value root;

	CStringA strNameA;

	strNameA = m_strName;

	std::string name;

	name = strNameA.GetBuffer(strNameA.GetLength());

	root["name"] = name;

	DstAddr.sin_family = AF_INET;
	DstAddr.sin_port = htons(nPort);

	m_ctrIp.GetAddress(dwIp);
	DstAddr.sin_addr.S_un.S_addr = htonl(dwIp);


	CStringA strSendA;
	strSendA = m_strSend;
	string data;

	data = strSendA.GetBuffer(strSendA.GetLength());

	root["data"] = data;

	root["type"] = "data";

	std::string text = root.toStyledString();

	sprintf(send, "%s", text.c_str());

	int nTemp = sendto(m_socket, send, strlen(send), 0, (sockaddr *)&DstAddr, nLen);

	if (nTemp > 0)
	{
		PrintSend(m_socket, send, DstAddr);
	}

}


void CServerDlg::OnBnClickedCheckServer()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
}


void CServerDlg::OnBnClickedButtonGetOnline()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	DWORD dwIp;


	P2POnlineList.Clear();

	m_PeerList.DeleteAllItems();


	char send[2048] = { 0 };

	Node * pNew = NULL;

	SOCKADDR_IN DstAddr = { 0 };
	int nLen = sizeof(DstAddr);

	SOCKADDR_IN self = { 0 };

	int nSelfLen = sizeof(self);

	UpdateData(TRUE);


	DstAddr.sin_family = AF_INET;
	DstAddr.sin_port = htons(m_nSvrPort);

	m_svrIp.GetAddress(dwIp);
	DstAddr.sin_addr.S_un.S_addr = htonl(dwIp);

	if (m_bServer)
	{
		AfxMessageBox(_T("server ����Ҫ!"));
		return;
	}

	Json::Value root;

	root["type"] = "list client";

	char buf[1024] = { 0 };
	
	sprintf(buf, "%s", root.toStyledString().c_str());

	int nTemp = sendto(m_socket, buf, strlen(buf), 0, (sockaddr *)&DstAddr, nLen);

	if (nTemp > 0)
	{
		PrintSend(m_socket, buf, DstAddr);	
	}

	

}



DWORD WINAPI OnLineThreadProc(_In_ LPVOID lpParameter)
{
	CServerDlg * pDlg = (CServerDlg *)lpParameter;

	if (!pDlg)
	{
		bStop = TRUE;
	}

	while (!bStop)
	{
		OnlineList.FlushList(100);
		if (pDlg)
		{
			if (pDlg->m_bServer)
				pDlg->PostMessage(WM_FLUSH_SERVER_LIST, 0, 0);
			else
			{
				pDlg->PostMessage(WM_FLUSH_CLIENT_LIST, 0, 0);
				if (!pDlg->m_strName.IsEmpty())
				{
					CStringA strTempA;
					strTempA = pDlg->m_strName;

					string name = strTempA;

					OnlineList.sendalive(pDlg->m_socket, name);
				}
			}
		}
		Sleep(100);
	}

	return 0;
}

DWORD WINAPI P2PThreadProc(_In_ LPVOID lpParameter)
{
	CServerDlg * pDlg = (CServerDlg *)lpParameter;

	if (!pDlg)
	{
		bP2PStop = TRUE;
	}

	while (!bP2PStop)
	{
		P2POnlineList.FlushList(100);
		if (pDlg)
		{
			if (!pDlg->m_strName.IsEmpty())
			{
				CStringA strTempA;
				strTempA = pDlg->m_strName;

				string name = strTempA;

				P2POnlineList.sendalive(pDlg->m_socket, name);
			}
		}
		
		Sleep(100);
	}

	return 0;
}

void CServerDlg::OnNMRClickListPeerOnline(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	


}


void CServerDlg::OnBnClickedButtonConnPeer()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SOCKADDR_IN DstAddr = { 0 };
	CStringA strPortA;
	CStringA strSendA;
	CString strCmd;
	CString strIp;
	CStringA strIpA;
	CString strPort;
	DWORD dwIp;
	CString strName;
	CStringA strNameA;

	SOCKADDR_IN self = { 0 };
	UpdateData(TRUE);

	int nSelfLen = sizeof(self);

	char send[1024] = { 0 };

	int nLen = sizeof(DstAddr);
	
	int nIndex = m_PeerList.GetSelectionMark();

	if (nIndex == -1)
		return;

	strName = m_PeerList.GetItemText(nIndex, 0);
	strNameA = strName;


	Json::Value root;

	root["type"] = "nat";
	//�Է�������
	root["dstname"] = strNameA.GetBuffer(strNameA.GetLength());
	//��������
	strNameA = m_strName;
	root["srcname"] = strNameA.GetBuffer(strNameA.GetLength());

	char buf[1024] = { 0 };

	sprintf(buf, "%s", root.toStyledString().c_str());

	DstAddr.sin_family = AF_INET;
	DstAddr.sin_port = htons(m_nSvrPort);
	m_svrIp.GetAddress(dwIp);
	DstAddr.sin_addr.S_un.S_addr = htonl(dwIp);

	int nTemp = sendto(m_socket, buf, strlen(buf), 0, (sockaddr *)&DstAddr, nLen);

	if (nTemp > 0)
	{
		PrintSend(m_socket, buf, DstAddr);
	}
}


void CServerDlg::OnBnClickedButtonLogin()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	DWORD dwIp;

	char send[2048] = { 0 };

	SOCKADDR_IN DstAddr = { 0 };

	int nLen = sizeof(DstAddr);

	UpdateData(TRUE);

	DstAddr.sin_family = AF_INET;
	DstAddr.sin_port = htons(nPort);

	m_ctrIp.GetAddress(dwIp);
	DstAddr.sin_addr.S_un.S_addr = htonl(dwIp);

	m_strName.Trim();

	if (m_strName.IsEmpty())
	{
		AfxMessageBox(_T("�������û���!"));
		return;
	}



	Json::Value root;

	CStringA strNameA;

	strNameA = m_strName;

	std::string name;

	name = strNameA.GetBuffer(strNameA.GetLength());

	root["name"] = name;

	root["type"] = "login";

	std::string text = root.toStyledString();

	sprintf(send,"%s", text.c_str());


	if (sendto(m_socket, text.c_str(), text.length(),0, (sockaddr *)&DstAddr, nLen) > 0)
	{
		PrintSend(m_socket, send, DstAddr);
	}

}


void CServerDlg::OnBnClickedRadioServer()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	SetDlgItemText(IDC_STATIC_NODELIST_CAP, _T("�ͻ������߽���б�"));
	SetWindowText(_T("������"));
	m_bServer = TRUE;
}


void CServerDlg::OnBnClickedRadioClient()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������


	SetDlgItemText(IDC_STATIC_NODELIST_CAP, _T("���������߽���б�"));
	SetWindowText(_T("�ͻ���"));
	m_bServer = FALSE;
}


void CServerDlg::OnBnClickedButtonP2pSend()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	char send[2048] = { 0 };

	UpdateData(TRUE);

	m_strName.Trim();

	int nIndex = m_PeerList.GetSelectionMark();
	if (nIndex == -1)
	{
		AfxMessageBox(_T("��ѡ��ͨ�Ž��"));
		return;
	}
	CString strIp;
	strIp = m_PeerList.GetItemText(nIndex, 0);
	CStringA strIpA;
	strIpA = strIp;

	std::string name;
	name = strIpA.GetBuffer(strIpA.GetLength());

	string ip;
	unsigned short port;

	SOCKADDR_IN DstAddr = { 0 };
	int nLen = sizeof(DstAddr);

	
	Json::Value root;

	if (P2POnlineList.find(name, ip, port))
	{

		SOCKADDR_IN DstAddr = { 0 };
		int nLen = sizeof(DstAddr);

		DstAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
		DstAddr.sin_port = htons(port);
		DstAddr.sin_family = AF_INET;

		CStringA strTempA;

		strTempA = m_strName;

		name = strTempA.GetBuffer(strTempA.GetLength());

		root["name"] = name;


		CStringA strSendA;
		strSendA = m_strSend;
		string data;

		data = strSendA.GetBuffer(strSendA.GetLength());

		root["data"] = data;

		root["type"] = "data";

		std::string text = root.toStyledString();

		sprintf(send, "%s", text.c_str());

		int nTemp = sendto(m_socket, send, strlen(send), 0, (sockaddr *)&DstAddr, nLen);

		if (nTemp > 0)
		{
			PrintSend(m_socket, send, DstAddr);
		}
	}
	else
	{
		AfxMessageBox(_T("���ֶ�����"));
	}




}
