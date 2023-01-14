
// ServerDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include <winsock2.h>
#include <list>
#include <map>
#include <string>
#include "afxwin.h"

#define SEND 0
#define RECV 1

typedef struct _tagNode
{
	char srcip[512];
	unsigned short srcport;
	char dstip[512];
	unsigned short dstport;
	int nDir;
	char data[1024];
	CString strTime;
}Node;

typedef struct _tagClientNode
{
	SOCKADDR_IN addr;
	int nCount;
	std::string name;
	int nState;
}CNode;


// CServerDlg 对话框
class CServerDlg : public CDialogEx
{
// 构造
public:
	CServerDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CServerDlg();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CListCtrl m_AddrList;
	CString m_strContent;

public:
	SOCKET m_socket;
public:
	int m_nListenPort;
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg LRESULT OnSockRead(WPARAM w, LPARAM l);
	afx_msg LRESULT OnFlushServerList(WPARAM w, LPARAM l);
	afx_msg LRESULT OnFlushClientList(WPARAM w, LPARAM l);


	std::list<Node *> lists;
	afx_msg void OnNMClickListIpPort(NMHDR *pNMHDR, LRESULT *pResult);

	CString GetData(CString strIp, CString strPort, CString DstIp, CString DstPort, CString strTime, int nIndex);
	CIPAddressCtrl m_ctrIp;
	afx_msg void OnBnClickedButtonSend();
	short nPort;
	CString m_strSend;
	BOOL m_bServer;
	
	afx_msg void OnBnClickedCheckServer();
	CListCtrl m_ActiveNodeList;
	afx_msg void OnBnClickedButtonGetOnline();
	CListCtrl m_PeerList;
	CIPAddressCtrl m_svrIp;
	int m_nSvrPort;
	afx_msg void OnNMRClickListPeerOnline(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonConnPeer();
	CString m_strName;
	afx_msg void OnBnClickedButtonLogin();

	void FlushPeerAddr(std::string name, std::string ip, unsigned int port);


	bool PrintfRecv(SOCKET s, char * buf, int nBufLen, SOCKADDR_IN & client);

	bool PrintSend(SOCKET s, char * send, SOCKADDR_IN & client);

	CButton m_btnServer;

	std::list<CNode> timerlist;
	std::list<CNode> p2ptimer;

	int m_nWorkMode;
	afx_msg void OnBnClickedRadioServer();
	afx_msg void OnBnClickedRadioClient();

	void OnTimer(UINT nIDEvent);

	void DeleteTimer(std::list<CNode> & list,std::string name);

	void SetPeerLink(std::string name);

	void SetTimerDelay(std::list<CNode> & list, std::string name, int nCount);

	void FlushTimer(std::list<CNode> & list, std::string name, std::string ip, unsigned short port);

	
	afx_msg void OnBnClickedButtonP2pSend();
};
