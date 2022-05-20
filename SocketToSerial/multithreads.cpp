
#include "framework.h"
#include "SocketToSerial.h"

#include "TabControl.h"

#include <Uxtheme.h>
#include <Commctrl.h>
#include <stdio.h>

#include "multithreads.h"

DWORD WINAPI threadSerialCom(LPVOID lpParam)
{
    LPCONNECTIONS pInfoConnections = (LPCONNECTIONS)lpParam;

    int iConnNum = pInfoConnections->iInternal;
    char szReadBuff[1024];
    DWORD dwBytesRead = 0;

    SetCommMask(pInfoConnections->hSerialPort[iConnNum], EV_RXCHAR);


    while(1)
    {
        BOOL Status = ReadFile(pInfoConnections->hSerialPort[iConnNum], &szReadBuff, 1024, &dwBytesRead, NULL);

        if (dwBytesRead > 0 && dwBytesRead < 1024)
        {
            szReadBuff[dwBytesRead] = '\0';

            WinDbgOut(1024, "%s\n", szReadBuff);

            HWND hEditOut = GetDlgItem(pInfoConnections->hDialogs[iConnNum], IDC_EDIT1);

            int index = GetWindowTextLength(hEditOut);
            SetFocus(hEditOut);
            SendMessageA(hEditOut, EM_SETSEL, (WPARAM)index, (LPARAM)index);
            SendMessageA(hEditOut, EM_REPLACESEL, 0, (LPARAM)szReadBuff);
            
        }
    }
}

void CriarThread(LPCONNECTIONS lpConnections, int iConexaoNum)
{
    DWORD dwThreadId;

    lpConnections->iInternal = iConexaoNum;

    lpConnections->hThread[iConexaoNum] = CreateThread( NULL, 0, threadSerialCom, lpConnections, 0, &dwThreadId);
}


void WriteSerialPort(LPCONNECTIONS lpConnections, int iLen, int iConexaoNum, const char* format, ...)
{
    va_list arg;
    va_start(arg, format);

    char* szBuffer = (char*)malloc(2 * (size_t)iLen);

    if (szBuffer != NULL) {
        vsprintf_s(szBuffer, iLen, format, arg);

        DWORD dwWritten = 0;

        SetCommMask(lpConnections->hSerialPort[iConexaoNum], EV_TXEMPTY);

        WriteFile(lpConnections->hSerialPort[iConexaoNum],
            szBuffer,
            lstrlen(szBuffer),
            &dwWritten,
            NULL);

        SetCommMask(lpConnections->hSerialPort[iConexaoNum], EV_RXCHAR);

        FlushFileBuffers(lpConnections->hSerialPort[iConexaoNum]);

    }
    free(szBuffer);
    va_end(arg);
}