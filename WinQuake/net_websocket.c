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

#include "net_websocket.h"

#define MAX_WS_MESSAGES 256 // size of an array using a unsigned char as an index
#define MAX_WS_MESSAGE_SIZE 65536
#define WEBSOCKET_URL "ws://localhost:7071/ws"
#define lowercaseuuid true

extern int gethostname(char *, int);
extern int close(int);

extern cvar_t hostname;
static int net_controlsocket;
static unsigned long myAddr;
static qboolean ws_opened = false;
static qboolean ws_onopen_handled = false;
static qboolean ws_onclose_handled = false;
static EMSCRIPTEN_WEBSOCKET_T ws;
static char *uuid;

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
} wsMessages = {0};

//=============================================================================

// credit to https://stackoverflow.com/a/71826534/7884214
char *gen_uuid()
{
	char v[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	// 3fb17ebc-bc38-4939-bc8b-74f2443281d4
	// 8 dash 4 dash 4 dash 4 dash 12
	static char buf[37] = {0};

	// gen random for all spaces because lazy
	for (int i = 0; i < 36; ++i)
	{
		buf[i] = v[rand() % 16];
	}

	// put dashes in place
	buf[8] = '-';
	buf[13] = '-';
	buf[18] = '-';
	buf[23] = '-';

	// needs end byte
	buf[36] = '\0';

	return buf;
}

//=============================================================================

EM_BOOL _WebSocket_onopen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData)
{
	ws_onopen_handled = true;

	Sys_Printf("_WebSocket_onopen: connected\n");

	char buf[MAX_WS_MESSAGE_SIZE];

	buf[0] = 62;
	buf[1] = 91;
	buf[2] = 27;
	buf[3] = 3;
	buf[4] = 20;
	buf[5] = 0;

	int offset = 6;

	int i;
	for (i = offset; i < MAX_WS_MESSAGE_SIZE; i++)
	{
		buf[i] = uuid[i - offset];
		if (buf[i] == '\x00')
			break;
	}

	struct qsockaddr addr;

	WebSocket_Write(ws, (byte *) &buf, i, &addr);

	return EM_TRUE;
}

EM_BOOL _WebSocket_onerror(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData)
{
	Sys_Printf("_WebSocket_onerror: failed (see browser console, hopefully)\n");

	return EM_TRUE;
}

EM_BOOL _WebSocket_onclose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData)
{
	if (!ws_onclose_handled)
		WebSocket_CloseSocket(ws);

	if (ws_opened)
	{
		Sys_Printf("_WebSocket_onclose: disconnected unexpectedly\n");
		Con_Printf("_WebSocket_onclose: disconnected unexpectedly\n");
	}
	else
	{
		Sys_Printf("_WebSocket_onclose: disconnected at user request\n");
	}

	ws_onclose_handled = true;
	return EM_TRUE;
}

EM_BOOL _WebSocket_onmessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData)
{
	if (websocketEvent->isText)
	{
		return EM_TRUE;
	}

	int delta = wsMessages.write_index - wsMessages.read_index;
	if (delta < 0)
	{
		delta += MAX_WS_MESSAGES;
	}

	if (wsMessages.write_index + 1 == wsMessages.read_index)
	{
		Sys_Printf("_WebSocket_onmessage: wsMessages buffer overflow (r: %d, w: %d, d: %d); disconnecting.\n", wsMessages.read_index, wsMessages.write_index, delta);
		Con_Printf("_WebSocket_onmessage: wsMessages buffer overflow (r: %d, w: %d, d: %d); disconnecting.\n", wsMessages.read_index, wsMessages.write_index, delta);
		WebSocket_CloseSocket(ws);
		return EM_FALSE;
	}

	wsMessages.messages[wsMessages.write_index].length = (unsigned int)websocketEvent->numBytes;
	memcpy(wsMessages.messages[wsMessages.write_index].data, websocketEvent->data, websocketEvent->numBytes);

	wsMessages.write_index++;

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
	{
		Sys_Printf("WebSocket_Init: -nowebsocket flag given (disabling WebSockets)\n");
		return -1;
	}

	if (!emscripten_websocket_is_supported())
	{
		Sys_Error("WebSocket_Init: emscripten_websocket_is_supported() says WebSockets aren't supported\n");
	}

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
	{
		Sys_Error("WebSocket_Init: Unable to open control socket\n");
	}

	WebSocket_GetSocketAddr(net_controlsocket, &addr);
	Q_strcpy(my_tcpip_address, WebSocket_AddrToString(&addr));
	colon = Q_strrchr(my_tcpip_address, ':');
	if (colon)
		*colon = 0;

	srand(time(NULL));
	uuid = malloc(37);
	uuid = gen_uuid();

	Con_Printf("WebSocket Initialized; uuid: %s\n", uuid);
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
	if (ws_opened) {
		return 0;
	}

	EmscriptenWebSocketCreateAttributes ws_attrs = {
		WEBSOCKET_URL,
		NULL,
		EM_TRUE,
	};

	ws_onopen_handled = false;
	ws_onclose_handled = false;

	for (int i = 0; i < MAX_WS_MESSAGES; i++)
	{
		wsMessages.messages[i].length = 0;
		wsMessages.messages[i].data[0] = '\x00';
	}

	wsMessages.read_index = 0;
	wsMessages.write_index = 0;

	if ((ws = emscripten_websocket_new(&ws_attrs)) <= 0)
	{
		Sys_Printf("WebSocket_OpenSocket: failed to open socket\n");
		Con_Printf("WebSocket_OpenSocket: failed to open socket\n");
		return -1;
	}

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

	ws_opened = false;

	char *reason = "Closed by WebSocket_CloseSocket";
	emscripten_websocket_close(ws, 1000, reason);

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

	unsigned char read_index = wsMessages.read_index;

	if (wsMessages.messages[read_index].length <= 0)
	{
		return 0;
	}

	unsigned int length = wsMessages.messages[read_index].length;
	memcpy(buf, wsMessages.messages[read_index].data, length);

	wsMessages.messages[read_index].length = 0;

	read_index++;
	wsMessages.read_index = read_index;

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
	sprintf(buffer, "%s", "6.9.42.0:13337");
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
	sprintf(name, "%s", "quake-wasm\x00");
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
