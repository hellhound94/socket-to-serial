#pragma once


void CriarThread(LPCONNECTIONS lpConnections, int iConexaoNum);

void WriteSerialPort(LPCONNECTIONS lpConnections, int iLen, int iConexaoNum, const char* format, ...);