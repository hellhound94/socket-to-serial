// SocketToSerial.cpp : Define o ponto de entrada para o aplicativo.
//
/*
    Software com o objetivo principal de ser um conversor de porta serial para TCPIP

    Inicialmente pretendo fazer ele receber/enviar dados por socket e replicar em uma porta serial.

    Atualmente ele está listando e conectando nas portas seriais disponíveis, podendo trocar informações normalmente pela porta conectada.

    Desenvolvido por: Abner Cordeiro - 20/05/2022

*/
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "framework.h"
#include "SocketToSerial.h"

#include "TabControl.h"

#include <Uxtheme.h>
#include <Commctrl.h>
#include <stdio.h>

#include "multithreads.h"

#define WINMAIN_WIDTH 800
#define WINMAIN_HEIGHT 745

#define MAX_LOADSTRING 100

// Variáveis Globais:
HINSTANCE hInst;                                // instância atual
char szTitle[MAX_LOADSTRING];                  // O texto da barra de título
char szWindowClass[MAX_LOADSTRING];            // o nome da classe da janela principal

// Declarações de encaminhamento de funções incluídas nesse módulo de código:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


CONNECTIONS g_MyConnections;

const char* szBaudRate[] =
{
    "110","300","600", "1200", "2400", "4800", "9600",
    "14400", "19200", "38400", "57600", "115200", "128000", "256000", ""
};

const char* szLenByte[] = { "8 bits","7 bits","6 bits" };

const char* szParity[] = { "No parity.","Odd parity.","Even parity.","Mark parity.","Space parity.","","","","" };

const char* szStopBit[] = { "1 stop bit.","1.5 stop bits.","2 stop bits." };

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Coloque o código aqui.

    // Inicializar cadeias de caracteres globais
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_SOCKETTOSERIAL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Realize a inicialização do aplicativo:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SOCKETTOSERIAL));

    MSG msg;

    // Loop de mensagem principal:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNÇÃO: MyRegisterClass()
//
//  FINALIDADE: Registra a classe de janela.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOCKETTOSERIAL));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_SOCKETTOSERIAL);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNÇÃO: InitInstance(HINSTANCE, int)
//
//   FINALIDADE: Salva o identificador de instância e cria a janela principal
//
//   COMENTÁRIOS:
//
//        Nesta função, o identificador de instâncias é salvo em uma variável global e
//        crie e exiba a janela do programa principal.
//

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Armazenar o identificador de instância em nossa variável global

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       (GetSystemMetrics(SM_CXSCREEN) - WINMAIN_WIDTH) / 2,
       (GetSystemMetrics(SM_CYSCREEN) - WINMAIN_HEIGHT) / 2,
       WINMAIN_WIDTH, WINMAIN_HEIGHT, nullptr, nullptr, hInstance, nullptr);


   if (!hWnd)
   {
      return FALSE;
   }
   

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


void WinDbgOut(int iLen, const char* format, ...)
{
    va_list arg;
    va_start(arg, format);

    char* szBuffer = (char*)malloc(2 * (size_t)iLen);

    if (szBuffer != NULL) {
        vsprintf_s(szBuffer, iLen, format, arg);

        OutputDebugString(szBuffer);
    }
    free(szBuffer);
    va_end(arg);
}

void PrintCommState(DCB dcb)
{
    WinDbgOut(1024, TEXT("\nBaudRate = %d, ByteSize = %d, Parity = %d, StopBits = %d\n"),
        dcb.BaudRate,
        dcb.ByteSize,
        dcb.Parity,
        dcb.StopBits);
}

BOOL CALLBACK PortDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    RECT windowPrincipal;
    int iPortIndex = 0;

    switch (message)
    {
    case WM_INITDIALOG:
        GetClientRect(hwndDlg, &rc);

        GetClientRect(g_MyConnections.hTabControl, &windowPrincipal);

        MoveWindow(hwndDlg,1,25, windowPrincipal.right - 8, windowPrincipal.bottom - 25,TRUE);
        ShowWindow(hwndDlg, SW_HIDE);

        for (int iCount = 0; iCount < 14; iCount++) {
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szBaudRate[iCount]);

        }
        for (int iCount = 0; iCount < 3; iCount++) {
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO2), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szLenByte[iCount]);

        }
        for (int iCount = 0; iCount < 3; iCount++) {
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO3), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szStopBit[iCount]);

        }

        for (int iCount = 0; iCount < 5; iCount++) {
            SendMessage(GetDlgItem(hwndDlg, IDC_COMBO4), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szParity[iCount]);

        }

        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), (UINT)CB_SETCURSEL, (WPARAM)11, (LPARAM)0);
        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO2), (UINT)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO3), (UINT)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
        SendMessage(GetDlgItem(hwndDlg, IDC_COMBO4), (UINT)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

        for (int iCount = 0; iCount < g_MyConnections.iNumPorts; iCount++)
        {
            if (g_MyConnections.hDialogs[iCount] == hwndDlg)
            {
                iPortIndex = iCount;
                break;
            }
        }

        //g_MyConnections.hEditControlIn [iPortIndex] = GetDlgItem(hwndDlg, IDC_EDIT2);
        //g_MyConnections.hEditControlOut[iPortIndex] = GetDlgItem(hwndDlg, IDC_EDIT1);

        break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Analise as seleções do menu:
        switch (wmId)
        {
        case IDC_BUTTON1:
        {
            char szBuff[1024];

            GetWindowText(GetDlgItem(hwndDlg, IDC_BUTTON1), szBuff,1024);

            if (szBuff[0] == 'C')
            {
                DWORD ucBaudRate = (DWORD)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                DWORD ucLenByte  = (DWORD)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO2), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                BYTE   ucStopBit = (BYTE)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO3), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                BYTE   ucParity  = (BYTE)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO4), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                DCB dcb;
                //HANDLE hCom;
                BOOL fSuccess;
                char pcCommPort[1024] = {};

                int iPortIndex = 0;

                for (int iCount = 0; iCount < g_MyConnections.iNumPorts; iCount++)
                {
                    if (g_MyConnections.hDialogs[iCount] == hwndDlg)
                    {
                        strcpy_s(pcCommPort, g_MyConnections.szSerialPortName[iCount]);

                        WinDbgOut(1024, "Conectando em: %s.\n", pcCommPort);

                        iPortIndex = iCount;
                        break;
                    }
                }

                if (lstrlen(pcCommPort) > 5)
                {
                    g_MyConnections.hSerialPort[iPortIndex] = CreateFile(pcCommPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    if (g_MyConnections.hSerialPort[iPortIndex] == INVALID_HANDLE_VALUE)
                    {
                        WinDbgOut(1024, "CreateFile falhou com o codigo de erro: %d.\n", GetLastError());
                        return (1);
                    }

                    SecureZeroMemory(&dcb, sizeof(DCB));
                    dcb.DCBlength = sizeof(DCB);

                    fSuccess = GetCommState(g_MyConnections.hSerialPort[iPortIndex], &dcb);

                    if (!fSuccess)
                    {
                        WinDbgOut(1024, "GetCommState falhou com o codigo de erro: %d.\n", GetLastError());
                        return (2);
                    }

                    DWORD dwBaudRate[] = { 110,300,600, 1200, 2400, 4800, 9600,14400, 19200, 38400, 57600, 115200, 128000, 256000 };

                    BYTE dwByteSize[] = { 8,7,6,5 };

                    PrintCommState(dcb);

                    dcb.BaudRate = dwBaudRate[ucBaudRate];
                    dcb.ByteSize = dwByteSize[ucLenByte];
                    dcb.Parity = ucParity;
                    dcb.StopBits = ucStopBit;

                    fSuccess = SetCommState(g_MyConnections.hSerialPort[iPortIndex], &dcb);

                    if (!fSuccess)
                    {
                        WinDbgOut(1024, "Falha em SetCommState com codigo de erro: %d.\n", GetLastError());
                        return (3);
                    }
    
                    COMMTIMEOUTS timeouts;
                    timeouts.ReadIntervalTimeout = 0;
                    timeouts.ReadTotalTimeoutMultiplier = 0;
                    timeouts.ReadTotalTimeoutConstant = 1;
                    timeouts.WriteTotalTimeoutMultiplier = 0;
                    timeouts.WriteTotalTimeoutConstant = 0;
                    SetCommTimeouts(g_MyConnections.hSerialPort[iPortIndex], &timeouts);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO2), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO3), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO4), FALSE);

                    SetWindowText(GetDlgItem(hwndDlg, IDC_BUTTON1), "Desconectar...");

                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT2), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON3), TRUE);

                    CriarThread(&g_MyConnections, iPortIndex);
                }
            }
            else if (szBuff[0] == 'D')
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO2), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO3), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO4), TRUE);

                SetWindowText(GetDlgItem(hwndDlg, IDC_BUTTON1), "Conectar...");

                EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT2), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON3), FALSE);

                int iPortIndex = 0;

                for (int iCount = 0; iCount < g_MyConnections.iNumPorts; iCount++)
                {
                    if (g_MyConnections.hDialogs[iCount] == hwndDlg)
                    {
                        WinDbgOut(1024, "Desconectando: %s.\n", g_MyConnections.szSerialPortName[iCount]);

                        iPortIndex = iCount;
                        break;
                    }
                }

                DWORD dwExitCode = 0;
                BOOL bExitCode = GetExitCodeThread(g_MyConnections.hThread[iPortIndex],&dwExitCode);

                WinDbgOut(1024, "Status Exit Code: %d\nTerminando thread: %d\n", bExitCode, TerminateThread(g_MyConnections.hThread[iPortIndex], bExitCode));

                CloseHandle(g_MyConnections.hSerialPort[iPortIndex]);
            }

        }
        break;

        case IDC_BUTTON3:
        {
            int iPortIndex = 0;

            for (int iCount = 0; iCount < g_MyConnections.iNumPorts; iCount++)
            {
                if (g_MyConnections.hDialogs[iCount] == hwndDlg)
                {
                    iPortIndex = iCount;
                    break;
                }
            }

            char szTextOut[1024];

            GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT2), szTextOut, 1024);
            WriteSerialPort(&g_MyConnections, 1024, iPortIndex, szTextOut);
        }
        break;
        }
    }
    break;
    }
    return FALSE;
}
//
//  FUNÇÃO: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  FINALIDADE: Processa as mensagens para a janela principal.
//
//  WM_COMMAND  - processar o menu do aplicativo
//  WM_PAINT    - Pintar a janela principal
//  WM_DESTROY  - postar uma mensagem de saída e retornar
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_MyConnections.hInstance = hInst;
        g_MyConnections.hPrincipalWin = hWnd;

        g_MyConnections.hTabControl = DoCreateTabControl(&g_MyConnections);

        for (int iCount = 0; iCount < g_MyConnections.iNumPorts; iCount++)
        {
            g_MyConnections.hDialogs[iCount] = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)PortDialogProc);
        }

        


        ShowWindow(g_MyConnections.hDialogs[0], SW_SHOW);
        

    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Analise as seleções do menu:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Adicione qualquer código de desenho que use hdc aqui...
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_NOTIFY:
    {
        if (((LPNMHDR)lParam)->code == (UINT)TCN_SELCHANGE)
        {
            if (((LPNMHDR)lParam)->idFrom == TABCONTROL1)
            {
                int iPage = TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);

                

                for (int iCount = 0; iCount < g_MyConnections.iNumPorts; iCount++)
                {
                    ShowWindow(g_MyConnections.hDialogs[iCount], SW_HIDE);
                }

                ShowWindow(g_MyConnections.hDialogs[iPage], SW_SHOW);
                
            }
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Manipulador de mensagem para a caixa 'sobre'.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
