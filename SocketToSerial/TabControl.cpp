/*
*
* Desenvolvido por Abner Cordeiro - 02/04/2022
* abnercordeiro@outlook.com
*
*/
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <Windowsx.h>
#include <Commctrl.h>
#include "SocketToSerial.h"
#include "TabControl.h"


// Creates a tab control, sized to fit the specified parent window's client
//   area, and adds some tabs. 
// Returns the handle to the tab control. 
// hwndParent - parent window (the application's main window). 
// 
HWND DoCreateTabControl(LPCONNECTIONS lpConnections)
{
	/* Variaveis variadas */
	RECT rcClient;
	INITCOMMONCONTROLSEX icex;
	HWND hwndTab;
	TCITEM tie;

	/* Inicializa a DLL Responsável pela beleza da bagaça */
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icex);

	/*
	 Pega o tamanho da janela principal,
	 assim calcumamos o tamanho que o tab control vai ocupar
	 */
	GetClientRect(lpConnections->hPrincipalWin, &rcClient);

	/* Criamos o TabControl */
	hwndTab = CreateWindow(WC_TABCONTROL, "",
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, rcClient.right, rcClient.bottom,
		lpConnections->hPrincipalWin, (HMENU)TABCONTROL1, lpConnections->hInstance, NULL);
	if (hwndTab == NULL) return NULL;


	char szTemp[1024];
	int iCountPort = 0;
	int iPorts = 0;

	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	tie.pszText = NULL;

	while (iCountPort < 255) {
		wsprintf(lpConnections->szSerialPortName[lpConnections->iNumPorts], "\\\\.\\COM%d", iCountPort);
		wsprintf(szTemp, "COM%d", iCountPort);

		lpConnections->hSerialPort[lpConnections->iNumPorts] = CreateFile(
			lpConnections->szSerialPortName[lpConnections->iNumPorts],
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (lpConnections->hSerialPort[lpConnections->iNumPorts] != INVALID_HANDLE_VALUE)
		{
			tie.pszText = (LPSTR)szTemp;

			/* Insere a pagina com a string correta */
			if (TabCtrl_InsertItem(hwndTab, lpConnections->iNumPorts, &tie) == -1)
			{
				/* Ops deu merda */
				DestroyWindow(hwndTab);
				return NULL;
			}

			CloseHandle(lpConnections->hSerialPort[lpConnections->iNumPorts]);

			lpConnections->iNumPorts++;
		}
		iCountPort++;
	}

	/* Seta uma fonte mais bonitinha, remova isso e veja o desastre */
	SendMessage(hwndTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);

	return hwndTab;
}


HRESULT TabControlOnSize(HWND hwndTab, LPARAM lParam)
{
	if (hwndTab == NULL)
		return E_INVALIDARG;

	if (!SetWindowPos(hwndTab, HWND_TOP, 0, 45, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), SWP_SHOWWINDOW))
		return E_FAIL;

	return S_OK;
}
