/*
Added by initialed85
*/
// net_websocket.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

extern int gethostname(char *, int);
extern int close(int);

extern cvar_t hostname;
static int net_controlsocket;
static unsigned long myAddr;
static qboolean ws_opened = false;
static qboolean ws_onopen_handled = false;
static qboolean ws_onclose_handled = false;
static EMSCRIPTEN_WEBSOCKET_T ws;

#define MAX_WS_MESSAGES 256 // size of an array using a unsigned char as an index
#define MAX_WS_MESSAGE_SIZE 65536

typedef struct
{
	unsigned int length;
	byte data[MAX_WS_MESSAGE_SIZE];
} WsMessage;

struct
{
	unsigned char read_index;
	unsigned char write_index;
	WsMessage messages[MAX_WS_MESSAGES];
} wsReceivedMessages = {0};

#define WEBSOCKET_URL "ws://localhost:8081/ws"

#include "net_websocket.h"

//=============================================================================

EM_BOOL _WebSocket_onopen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData)
{
	ws_onopen_handled = true;
	return EM_TRUE;
}

EM_BOOL _WebSocket_onerror(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData)
{
	return EM_TRUE;
}

EM_BOOL _WebSocket_onclose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData)
{
	if (!ws_onclose_handled)
		WebSocket_CloseSocket(ws);

	ws_onclose_handled = true;
	return EM_TRUE;
}

EM_BOOL _WebSocket_onmessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData)
{
	if (websocketEvent->isText)
	{
		return EM_TRUE;
	}

	unsigned char write_index = wsReceivedMessages.write_index;

	wsReceivedMessages.messages[write_index].length = (unsigned int)websocketEvent->numBytes;
	memcpy(wsReceivedMessages.messages[write_index].data, websocketEvent->data, websocketEvent->numBytes);

	write_index++;
	wsReceivedMessages.write_index = write_index;

	return EM_TRUE;
}

//=============================================================================

int WebSocket_Init(void)
{
	struct hostent *local;
	char buff[MAXHOSTNAMELEN];
	struct qsockaddr addr;
	char *colon;

	if (COM_CheckParm("-nowebsocket"))
		return -1;

	if (!emscripten_websocket_is_supported())
		Sys_Error("WebSocket_Init: emscripten_websocket_is_supported() says WebSockets aren't supported\n");

	// determine my name & address
	gethostname(buff, MAXHOSTNAMELEN);
	local = gethostbyname(buff);
	myAddr = *(int *)local->h_addr_list[0];

	// if the quake hostname isn't set, set it to the machine name
	if (Q_strcmp(hostname.string, "UNNAMED") == 0)
	{
		buff[15] = 0;
		Cvar_Set("hostname", buff);
	}

	if ((net_controlsocket = WebSocket_OpenSocket(0)) == -1)
		Sys_Error("WebSocket_Init: Unable to open control socket\n");

	WebSocket_GetSocketAddr(net_controlsocket, &addr);
	Q_strcpy(my_tcpip_address, WebSocket_AddrToString(&addr));
	colon = Q_strrchr(my_tcpip_address, ':');
	if (colon)
		*colon = 0;

	Con_Printf("WebSocket Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void WebSocket_Shutdown(void)
{
	WebSocket_Listen(false);
	WebSocket_CloseSocket(net_controlsocket);
}

//=============================================================================

void WebSocket_Listen(qboolean state)
{
}

//=============================================================================

int WebSocket_OpenSocket(int port)
{
	EmscriptenWebSocketCreateAttributes ws_attrs = {
		WEBSOCKET_URL,
		NULL,
		EM_TRUE,
	};

	ws_onopen_handled = false;
	ws_onclose_handled = false;

	if ((ws = emscripten_websocket_new(&ws_attrs)) <= 0)
		return -1;

	ws_opened = true;

	emscripten_websocket_set_onopen_callback(ws, NULL, _WebSocket_onopen);
	emscripten_websocket_set_onerror_callback(ws, NULL, _WebSocket_onerror);
	emscripten_websocket_set_onclose_callback(ws, NULL, _WebSocket_onclose);
	emscripten_websocket_set_onmessage_callback(ws, NULL, _WebSocket_onmessage);

	return ws;
}

//=============================================================================

int WebSocket_CloseSocket(int socket)
{
	if (!ws_opened)
	{
		return 0;
	}

	EMSCRIPTEN_RESULT res;
	char *reason = "Closed by WebSocket_CloseSocket";
	emscripten_websocket_close(ws, 1000, reason);

	ws_opened = false;

	return 0;
}

//=============================================================================

int WebSocket_Connect(int socket, struct qsockaddr *addr)
{
	while (!ws_onopen_handled)
	{
		emscripten_sleep(100);
	}

	return 0;
}

//=============================================================================

int WebSocket_CheckNewConnections(void)
{
	return -1;
}

//=============================================================================

int WebSocket_Read(int socket, byte *buf, int len, struct qsockaddr *addr)
{
	if (!ws_opened)
	{
		return -1;
	}

	if (!ws_onopen_handled)
	{
		return 0;
	}

	unsigned char read_index = wsReceivedMessages.read_index;

	if (wsReceivedMessages.messages[read_index].length <= 0)
	{
		return 0;
	}

	unsigned int length = wsReceivedMessages.messages[read_index].length;
	memcpy(buf, wsReceivedMessages.messages[read_index].data, length);

	wsReceivedMessages.messages[read_index].length = 0;

	read_index++;
	wsReceivedMessages.read_index = read_index;

	return length;
}

//=============================================================================

int WebSocket_MakeSocketBroadcastCapable(int socket)
{
	return 0;
}

//=============================================================================

int WebSocket_Broadcast(int socket, byte *buf, int len)
{
	return 0;
}

//=============================================================================

int WebSocket_Write(int socket, byte *buf, int len, struct qsockaddr *addr)
{
	if (!ws_opened)
	{
		return -1;
	}

	if (!ws_onopen_handled)
	{
		return 0;
	}

	EMSCRIPTEN_RESULT res;

	if ((res = emscripten_websocket_send_binary(ws, buf, (uint32_t)len)) < 0)
	{
		return -1;
	}

	return len;
}

//=============================================================================

char *WebSocket_AddrToString(struct qsockaddr *addr)
{
	static char buffer[22];
	int haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d");
	return buffer;
}

//=============================================================================

int WebSocket_StringToAddr(char *string, struct qsockaddr *addr)
{
	int ha1, ha2, ha3, ha4, hp;
	int ipaddr;

	sscanf("6.9.42.0:13337", "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons(hp);
	return 0;
}

//=============================================================================

int WebSocket_GetSocketAddr(int socket, struct qsockaddr *addr)
{
	WebSocket_StringToAddr("", addr);

	return 0;
}

//=============================================================================

int WebSocket_GetNameFromAddr(struct qsockaddr *addr, char *name)
{
	name = "quake-wasm\x00";
	return 0;
}

//=============================================================================

int WebSocket_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	WebSocket_StringToAddr("", addr);

	return 0;
}

//=============================================================================

int WebSocket_AddrCompare(struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	return 0;
}

//=============================================================================

int WebSocket_GetSocketPort(struct qsockaddr *addr)
{
	return 13337;
}

int WebSocket_SetSocketPort(struct qsockaddr *addr, int port)
{
	return 0;
}

//=============================================================================
