#pragma once

#include "resource.h"


typedef struct tagCONNECTIONS {
	HINSTANCE hInstance;
	HWND hPrincipalWin;
	HWND hTabControl;

	int iNumPorts;
	HANDLE hSerialPort[255];
	HWND hEditControlOut[255];
	HWND hEditControlIn[255];
	HWND hDialogs[255];
	char szSerialPortName[255][100];

	HANDLE hThread[255];
	
	int iInternal;


}CONNECTIONS,*LPCONNECTIONS;


void WinDbgOut(int iLen, const char* format, ...);