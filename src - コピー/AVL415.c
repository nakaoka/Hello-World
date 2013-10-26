#include <stdio.h>
#include "AVL415.h"

//----------------------------------------------------------------------
//
//  関数名   : 
//
//  機能     : 
//
//  機能説明 : 
//
//  返り値   : 
//
//  備考     : 
//             
//             
//----------------------------------------------------------------------
char RxData[1024];
char TxData[1024];
char MsgTxData[1024];
int RxData_Len = 0;
int CmdLen = 0;

char STS_ans[32];
char STS_cmd[32] = "SRDY";
float fSetSEC=0;
float fSEC=0;
float fCC=0;

#define CMD_ERR_STRING		"bad cmd"
typedef struct _CMDTBL {
	char			*Cmd;			
	char			*CmdRetAns;
	char			*Msg;
	int				data;			// 1:ステータス機能コード
	int				delay;			// delayがあればdelya回ビージーにする
} CMDTBL;

enum {
	enumSREM = 1,
	enumSxxx,
	enumExxx,
	enumAxxx,
	enumASTZ,
};

CMDTBL cmdtbl[] = {
	{	"SREM",		"\02 SREM 0 K0OK\03",				"SREM:リモートモード",		enumSREM,	10},
	{	"SRDY",		"\02 SRDY 0 K0OK\03",				"SRDY:レディ",				enumSxxx,	10},
	{	"SASB",		"\02 SASB 0 K0OK\03",				"SASB:サンプリングレディ",	enumSxxx,	10},
	{	"SMES",		"\02 SMES 0 K0OK\03",				"SMES:計測開始",			enumSxxx,	10},
//
	{	"EMZY",		"\02 EMZY 0 K0OK\03",				"EMZY:測定サイクル設定",	enumExxx,	0},
//
	{	"ASTF",		"\02 ASTF 1 K0BS 20\03",			"ASTF:エラー番号読込",		enumAxxx,	0},
	{	"AIZU",		"\02 AIZU 0 K0OK R 1 3 Z 1000 1 0 0 0 1\03","AIZU:内部ステータス読込",	enumAxxx,	0},
	{	"ASTZ",		"\02 ASTZ 0 ",						"ASTZ:ステータス読込",		enumASTZ,	0},
	{	"AFSN",		"\02 AFSN 0 K0RY 2 332 322 342\03",	"AFSN:すす濃度読込",		enumAxxx,	0},
	{	"AKON",		"\02 AKON 0 K0RY 2 100 102 104\03",	"AKON:すす濃度読込",		enumAxxx,	0},
	{	"APOL",		"\02 APOL 0 K0RY 2 200 202 204\03",	"APOL:汚染レベル読込",		enumAxxx,	0},
	{	"AMZY",		"\02 AMZY 0 K0RY v 20000 2\03",		"AMZY:測定サイクル定義",	enumAxxx,	0},
//
	{	NULL,	NULL,	NULL,	0}
};

//----------------------------------------------------------------------
//
//  関数名   : 
//
//  機能     : 
//
//  機能説明 : 
//
//  返り値   : 
//
//  備考     : 
//             
//             
//----------------------------------------------------------------------
void DecodeTempData(HWND hWnd)
{
	int a,b,c,d;
	double fval;
	char mod;
	char mod32;

	if( RxData_Len == 7 )
	{
		// 57600,N.8,1 (57600,パリティなし,8データビット,1ストップピット)
		//
		//  0  1  2  3  4  5  6
		// ① ② ③ ④ ⑤ ⑥ ⑦
		// A8 01 00 08 D9 FF 8A
		//          ~~ ~~
		//          |   |-LO-DATA
		//          HI-DATA
		//
		// ①START(0xA8)
		// ②TYPE            0x00:TK-62 / 0x01:TK-60
		// ③MOD    BIT7-4=  K:0000 　=J:0001
		//          BIT3-2=  通常:00 / ホールド:01 / MAX:10 / MIN:11 
		//          BIT1  =  電源モードオートパワーオフ = 0 / オートパワーオフ解除(Manual) = 1
		//          BIT0  =  電源    通常 = 0 / 電圧低下 = 1
		// ⑥CHECK  0xFF
		// ⑦END(0x8A)

		if( (RxData[0] & 0x0ff) == ASCII_A8 && (RxData[6] & 0x0ff) == ASCII_8A ){

			mod = (RxData[2] & 0x0c); 
			mod32 = mod >>2;

			if( mod32 == 0x01 ){
				sprintf(TxData, "\r\nHOLD\r\n");
				WriteTTYBlock( hWnd, TxData, strlen(TxData) );
			}
			if( mod32 == 0x02 ){
				sprintf(TxData, "\r\nMAX\r\n");
				WriteTTYBlock( hWnd, TxData, strlen(TxData) );
			}
			if( mod32 == 0x03 ){
				sprintf(TxData, "\r\nMIN\r\n");
				WriteTTYBlock( hWnd, TxData, strlen(TxData) );
			}
			
			//data
			if( (RxData[3] & 0x0ff) == 0x0ff && (RxData[4] & 0x0ff) == 0x0ff ){
				sprintf(TxData, "\r\n断線(オーバーレンジ)\r\n");
				WriteTTYBlock( hWnd, TxData, strlen(TxData) );			

			}else{
				a = (RxData[3] & 0x0ff);
				b = (RxData[4] & 0x0f0);
				b >>= 4;
				c = (RxData[4] & 0x00f);

				d = (a * 256) + (b * 16) + c;	//bit
				fval = d *0.1 -200.0;

				//
				sprintf(MsgTxData, "a=%02x, b=%02x, c=%02x d=%d, TEMP=%f", a, b, c, d, fval);
				OutputDebugString(MsgTxData);

				sprintf(TxData, "\r\nTEMP=%f\r\n", fval);
				WriteTTYBlock( hWnd, TxData, strlen(TxData) );
			}
		}

		return;
	}
}

//----------------------------------------------------------------------
//
//  関数名   : 
//
//  機能     : 
//
//  機能説明 : 
//
//  返り値   : 
//
//  備考     : 
//             
//             
//----------------------------------------------------------------------
void Chk_CmdString(HWND hWnd)
{
	int i, find = -1;
//	int No, len;
	static float fval = 0;
	static int busy = 0;

	//解読
	DecodeTempData(hWnd);

	return;



	for(i=0; cmdtbl[i].Cmd != NULL; i++){
//		if( strcmp((char*)RxData ,&cmdtbl[i].Cmd[0]) == 0 ){
		if( strncmp((char*)&RxData[2] ,cmdtbl[i].Cmd,4) == 0 ){
//			len = strlen(&cmdtbl[i].Cmd[0]);
//			if( strlen(RxData) == len ){
				// 見つかった
				find = i;
				break;
//			}
		}
	}

	memset(TxData, 0 , sizeof(TxData));
	if( find == -1 ){
		// おかしいコマンド
		sprintf(TxData, "\02 ???\03");

	}else{
		sprintf(MsgTxData, "\n%s", &cmdtbl[find].Msg[0]);
		if( cmdtbl[i].data == enumSxxx ){		// ステータスコマンド？
			busy = cmdtbl[find].delay;
			strcpy(STS_cmd,cmdtbl[find].Cmd);
			strcpy(STS_ans,cmdtbl[find].CmdRetAns);
		}
		if( cmdtbl[i].data == enumASTZ ){		// ステータス取得？
			strcpy(STS_ans,cmdtbl[find].CmdRetAns);
			if( busy ){
				busy--;
				strcat(STS_ans,"K0BS ");
			}else{
				strcat(STS_ans,"K0RY ");
				if( strcmp(STS_cmd,"SMES") == 0 ){
					strcpy(STS_cmd,"SRDY");
				}
			}

			strcat(STS_ans,STS_cmd);
			strcat(STS_ans,"\03");
		}

		if( cmdtbl[find].data > 0 ){
			// データ関係
			switch(cmdtbl[find].data){
			case enumSxxx:
			case enumASTZ:
				strcpy(TxData, STS_ans);
				break;
			case enumSREM:
			case enumExxx:
			case enumAxxx:
				strcpy(TxData, cmdtbl[find].CmdRetAns);
				break;
			}
		}else{
			strcpy(TxData, &cmdtbl[find].CmdRetAns[0]);

		}
	}

OutputDebugString(MsgTxData);

	if( strlen(TxData) >0 ){
		WriteCommBlock( hWnd, TxData, strlen(TxData) );
		strcat(TxData,"\r\n");
		WriteTTYBlock( hWnd, TxData, strlen(TxData) );
	}
}

//----------------------------------------------------------------------
//
//  関数名   : 
//
//  機能     : 
//
//  機能説明 : 
//
//  返り値   : 
//
//  備考     : 
//             
//             
//----------------------------------------------------------------------
int Get_CmdString(LPSTR lpBlock, int nLength )
{
//char buf[256];
	int i;
	for (i = 0 ; i < nLength; i++){
		switch (lpBlock[ i ] & 0x0ff){

			/*
			case ASCII_A8:
				break;
			*/
			case ASCII_8A:
					RxData[CmdLen] = (char)ASCII_8A;
					RxData[CmdLen+1] = '\0';
					RxData_Len = CmdLen + 1;
					CmdLen = 0;
					return 0;
					break;

//			case ASCII_STX:
/*
			case ASCII_ETX:
					RxData[CmdLen] = '\0';
					RxData_Len = CmdLen;
					CmdLen = 0;
					return 0;
					break;
*/
			default:
				RxData[CmdLen++] = lpBlock[ i ] ;
				break;
		}

//sprintf(buf,"%02x",lpBlock[i]& 0x0ff);
//OutputDebugString(buf);
//sprintf(buf,"len=%d",RxData_Len);
//OutputDebugString(buf);
	}
	return -1;
}
