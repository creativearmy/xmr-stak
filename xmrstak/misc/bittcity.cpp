#include <string>
#include <iostream>

#include "xmrstak/rapidjson/document.h"
#include "xmrstak/rapidjson/stringbuffer.h"
#include "xmrstak/rapidjson/writer.h"

#include "xmrstak/backend//globalStates.hpp"

#define CORE_MANAGER_IP "127.0.0.1"
#define CORE_MANAGER_PORT 3000

#ifdef _WIN32
#include "winsock2.h"  
#pragma comment(lib, "ws2_32.lib")  
#endif

using namespace std;

namespace xmrstak
{
	// hexdump source: http://stahlworks.com/dev/index.php?tool=csc01
	void hexdump(void *pAddressIn, long  lSize)
	{
		char szBuf[100];
		long lIndent = 1;
		long lOutLen, lIndex, lIndex2, lOutLen2;
		long lRelPos;
		struct { char *pData; unsigned long lSize; } buf;
		unsigned char *pTmp, ucTmp;
		unsigned char *pAddress = (unsigned char *)pAddressIn;

		buf.pData = (char *)pAddress;
		buf.lSize = lSize;

		while (buf.lSize > 0)
		{
			pTmp = (unsigned char *)buf.pData;
			lOutLen = (int)buf.lSize;
			if (lOutLen > 16)
				lOutLen = 16;

			// create a 64-character formatted output line:
			sprintf(szBuf, " >                            "
				"                      "
				"    %08lX", pTmp - pAddress);
			lOutLen2 = lOutLen;

			for (lIndex = 1 + lIndent, lIndex2 = 53 - 15 + lIndent, lRelPos = 0;
				lOutLen2;
				lOutLen2--, lIndex += 2, lIndex2++
				)
			{
				ucTmp = *pTmp++;

				sprintf(szBuf + lIndex, "%02X ", (unsigned short)ucTmp);
				if (!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
				szBuf[lIndex2] = ucTmp;

				if (!(++lRelPos & 3))     // extra blank after 4 bytes
				{
					lIndex++; szBuf[lIndex + 2] = ' ';
				}
			}

			if (!(lRelPos & 3)) lIndex--;

			szBuf[lIndex] = '<';
			szBuf[lIndex + 1] = ' ';

			fprintf(stderr, "%s\n", szBuf);

			buf.pData += lOutLen;
			buf.lSize -= lOutLen;
		}
	}

	bool socket_send(std::string data)
	{
#ifdef _WIN32
		WSADATA         wsd;
		SOCKET          sHost;
		SOCKADDR_IN     servAddr;
		int             retVal;

		if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
		{
			cout << "WSAStartup failed!" << endl;
			return false;
		}

		sHost = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sHost)
		{
			cout << "socket failed!" << endl;
			WSACleanup();
			return  false;
		}

		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = inet_addr(CORE_MANAGER_IP);
		servAddr.sin_port = htons((short)CORE_MANAGER_PORT);
		int nServAddlen = sizeof(servAddr);

		retVal = connect(sHost, (LPSOCKADDR)&servAddr, sizeof(servAddr));
		if (SOCKET_ERROR == retVal)
		{
			cout << "connect failed!" << endl;
			closesocket(sHost);
			WSACleanup();
			return -1;
		}
		
		retVal = send(sHost, data.c_str(), data.size(), 0);
		if (SOCKET_ERROR == retVal)
		{
			cout << "send failed!" << endl;
			closesocket(sHost);
			WSACleanup();
			return false;
		}

		//RecvLine(sHost, bufRecv);  
		//recv(sHost, bufRecv, 5, 0);
		
		closesocket(sHost);
		WSACleanup();
		return true;
#endif
	}
	
	void core_nonce(uint32_t nonce, bool eureka)
	{
		// Found nonce! Send  IPC over to Core Manager, to be forwarded to Node Agent and P2P Node if eureka set
		rapidjson::Document json;
		json.SetObject();
		rapidjson::Value value_str(rapidjson::kStringType);
		rapidjson::Value value_num(rapidjson::kNumberType);
		
		value_str.SetString("core", strlen("core"));
		json.AddMember("obj", value_str, json.GetAllocator());
		
		value_str.SetString("nonce", strlen("nonce"));
		json.AddMember("act", value_str, json.GetAllocator());

		value_str.SetString(globalStates::inst().oGlobalWork.sJobID, strlen(globalStates::inst().oGlobalWork.sJobID));
		json.AddMember("work", value_str, json.GetAllocator());

		value_num.SetUint(nonce);
		json.AddMember("nonce", value_num, json.GetAllocator());
		
		value_num.SetUint(1);
		if (!eureka) value_num.SetUint(0);
		json.AddMember("eureka", value_num, json.GetAllocator());
		
		// Serialize the JSON object
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		json.Accept(writer);
		
		std::string stringified = buffer.GetString();
		socket_send(stringified);
	}
	
	void core_done(uint32_t nonce_from, uint32_t nonce_count)
	{
		// Before quit, send notification to Core Manager
		rapidjson::Document json;
		json.SetObject();
		rapidjson::Value value_str(rapidjson::kStringType);
		rapidjson::Value value_num(rapidjson::kNumberType);

		value_str.SetString("core", strlen("core"));
		json.AddMember("obj", value_str, json.GetAllocator());

		value_str.SetString("done", strlen("done"));
		json.AddMember("act", value_str, json.GetAllocator());

		value_num.SetUint(nonce_from);
		json.AddMember("nonce_from", value_num, json.GetAllocator());

		value_num.SetInt(nonce_count);
		json.AddMember("count", value_num, json.GetAllocator());

		// Serialize the JSON object
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		json.Accept(writer);

		std::string stringified = buffer.GetString();
		socket_send(stringified);
	}

} // namepsace xmrstak
