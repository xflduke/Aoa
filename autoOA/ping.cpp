//
// PING.C -- ICMPと伏ソケットを聞喘するPingプログラム
//

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>

#include "ping.h"

// 坪何�v方
void ReportError(LPCSTR pstrFrom);
int  WaitForEchoReply(SOCKET s);
u_short in_cksum(u_short *addr, int len);

// ICMPエコ�`勣箔/鬴韈v方
int		SendEchoRequest(SOCKET, LPSOCKADDR_IN);
DWORD	RecvEchoReply(SOCKET, LPSOCKADDR_IN, u_char *);

// Ping()
// SendEchoRequest()とRecvEchoReply()を
// 柵び竃して�Y惚を竃薦する
BOOL Ping(LPCSTR pstrHost)
{
	SOCKET	  rawSocket;
	LPHOSTENT lpHost;
	struct    sockaddr_in saDest;
	struct    sockaddr_in saSrc;
	DWORD	  dwTimeSent;
	DWORD	  dwElapsed;
	u_char    cTTL;
	int       nLoop;
	int       nRet;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);

	// WinSockの兜豚晒
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr, "\nError initializing WinSock\n");
		return FALSE;
	}

	// バ�`ジョンのチェック
	if (wsaData.wVersion != wVersionRequested)
	{
		fprintf(stderr, "\nWinSock version not supported\n");
		WSACleanup();
		return FALSE;
	}

	// WinSockの兜豚晒
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr, "\nError initializing WinSock\n");
		WSACleanup();
		return FALSE;
	}

	// 伏ソケットの恬撹
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rawSocket == SOCKET_ERROR)
	{
		ReportError("socket()");
		WSACleanup();
		return FALSE;
	}

	// ホストの�碧�
	lpHost = (LPHOSTENT)gethostbyname(pstrHost);
	if (lpHost == NULL)
	{
		fprintf(stderr, "\nHost not found: %s\n", pstrHost);
		WSACleanup();
		return FALSE;
	}

	// 乱枠ソケットアドレスの�O協
	saDest.sin_addr.s_addr = *((u_long FAR *) (lpHost->h_addr));
	saDest.sin_family = AF_INET;
	saDest.sin_port = 0;

	// �嘛�彜�rをユ�`ザ�`に燕幣
	//printf("\nPinging %s [%s] with %d bytes of data:\n",
	//	pstrHost,
	//	inet_ntoa(saDest.sin_addr),
	//	REQ_DATASIZE);

	// Pingを採業か�g佩
	for (nLoop = 0; nLoop < 1; nLoop++)
	{
		// ICMPエコ�`勣箔を僕佚
		SendEchoRequest(rawSocket, &saDest);

		// select()を聞喘してデ�`タの鞭佚を棋�C
		nRet = WaitForEchoReply(rawSocket);
		if (nRet == SOCKET_ERROR)
		{
			ReportError("select()");
			WSACleanup();
			return FALSE;
		}
		if (!nRet)
		{
			printf("\nTimeOut");
			WSACleanup();
			return FALSE;
		}

		// 鬴陲鯤榻�
		dwTimeSent = RecvEchoReply(rawSocket, &saSrc, &cTTL);

		// �U�^�r�gを��麻
		//dwElapsed = GetTickCount() - dwTimeSent;
		//printf("\nReply from: %s: bytes=%d time=%ldms TTL=%d",
		//	inet_ntoa(saSrc.sin_addr),
		//	REQ_DATASIZE,
		//	dwElapsed,
		//	cTTL);
	}
	printf("\n");
	nRet = closesocket(rawSocket);
	if (nRet == SOCKET_ERROR)
		ReportError("closesocket()");

	// WinSockを盾慧
	WSACleanup();

	return TRUE;
}


// SendEchoRequest()
// エコ�`勣箔ヘッダ�`に秤�鵑�
// �O協し、乱枠に僕佚する
int SendEchoRequest(SOCKET s, LPSOCKADDR_IN lpstToAddr)
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// エコ�`勣箔に秤�鵑鰓O協
	echoReq.icmpHdr.Type = ICMP_ECHOREQ;
	echoReq.icmpHdr.Code = 0;
	echoReq.icmpHdr.Checksum = 0;
	echoReq.icmpHdr.ID = nId++;
	echoReq.icmpHdr.Seq = nSeq++;

	// 僕佚デ�`タを�O協
	for (nRet = 0; nRet < REQ_DATASIZE; nRet++)
		echoReq.cData[nRet] = ' ' + nRet;

	// 僕佚�rのティックカウントを隠贋
	echoReq.dwTime = GetTickCount();

	// パケット坪にデ�`タを秘れ、チェックサムを��麻
	echoReq.icmpHdr.Checksum = in_cksum((u_short *)&echoReq, sizeof(ECHOREQUEST));

	// エコ�`勣箔を僕佚
	nRet = sendto(s,						// ソケット
		(LPSTR)&echoReq,			// バッファ
		sizeof(ECHOREQUEST),
		0,							// フラグ
		(LPSOCKADDR)lpstToAddr, // 乱枠
		sizeof(SOCKADDR_IN));   // アドレスの�Lさ

	if (nRet == SOCKET_ERROR)
		ReportError("sendto()");
	return (nRet);
}


// RecvEchoReply()
// 彭佚デ�`タを鞭佚して、
// フィ�`ルド�eに盾裂する
DWORD RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL)
{
	ECHOREPLY echoReply;
	int nRet;
	int nAddrLen = sizeof(struct sockaddr_in);

	// エコ�`鬴陲鯤榻�
	nRet = recvfrom(s,					// ソケット
		(LPSTR)&echoReply,	// バッファ
		sizeof(ECHOREPLY),	// バッファのサイズ
		0,					// フラグ
		(LPSOCKADDR)lpsaFrom,	// 僕佚圷アドレス
		&nAddrLen);			// アドレス�Lへのポインタ

							// ��り�､鬟船Д奪�
	if (nRet == SOCKET_ERROR)
		ReportError("recvfrom()");

	// 僕佚�rとIP TTL�┫羮A�r�g��を卦す
	*pTTL = echoReply.ipHdr.TTL;
	return(echoReply.echoRequest.dwTime);
}

// �k伏した�嘛�の�鷂�
void ReportError(LPCSTR pWhere)
{
	fprintf(stderr, "\n%s error: %d\n",
		WSAGetLastError());
}


// WaitForEchoReply()
// select()を聞喘して、デ�`タが
// �iみ函り棋�C嶄かどうかを登�eする
int WaitForEchoReply(SOCKET s)
{
	struct timeval Timeout;
	fd_set readfds;

	readfds.fd_count = 1;
	readfds.fd_array[0] = s;
	Timeout.tv_sec = 5;
	Timeout.tv_usec = 0;

	return(select(1, &readfds, NULL, NULL, &Timeout));
}


//
// Mike Muuss' in_cksum() function
// and his comments from the original
// ping program
//
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983

/*
*			I N _ C K S U M
*
* Checksum routine for Internet Protocol family headers (C Version)
*
*/
u_short in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	*  Our algorithm is simple, using a 32 bit accumulator (sum),
	*  we add sequential 16 bit words to it, and at the end, fold
	*  back all the carry bits from the top 16 bits into the lower
	*  16 bits.
	*/
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w;
		sum += u;
	}

	/*
	* add back carry outs from top 16 bits to low 16 bits
	*/
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}