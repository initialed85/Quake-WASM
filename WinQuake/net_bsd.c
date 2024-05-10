/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

#include "net_loop.h"
#include "net_dgrm.h"

net_driver_t net_drivers[MAX_NET_DRIVERS] =
{
	{
	"Loopback",
	false,
	Loop_Init,
	Loop_Listen,
	Loop_SearchForHosts,
	Loop_Connect,
	Loop_CheckNewConnections,
	Loop_GetMessage,
	Loop_SendMessage,
	Loop_SendUnreliableMessage,
	Loop_CanSendMessage,
	Loop_CanSendUnreliableMessage,
	Loop_Close,
	Loop_Shutdown
	}
	,
	{
	"Datagram",
	false,
	Datagram_Init,
	Datagram_Listen,
	Datagram_SearchForHosts,
	Datagram_Connect,
	Datagram_CheckNewConnections,
	Datagram_GetMessage,
	Datagram_SendMessage,
	Datagram_SendUnreliableMessage,
	Datagram_CanSendMessage,
	Datagram_CanSendUnreliableMessage,
	Datagram_Close,
	Datagram_Shutdown
	}
};

int net_numdrivers = 2;

#include "net_udp.h"
#include "net_websocket.h"

net_landriver_t	net_landrivers[MAX_NET_DRIVERS] =
{
	// TODO: commented out by initialed85- Quake iterates all the lan drivers
	// to find something to connect to; this causes some argumentation between
	// the concrete WebSocket implementation in udp_websocket.c and the implicit
	// translation of BSD sockets to WebSocket that Emscripten does (which isn't
	// quite fit for our purpose in this case)
	// {
	// "UDP",
	// false,
	// 0,
	// UDP_Init,
	// UDP_Shutdown,
	// UDP_Listen,
	// UDP_OpenSocket,
	// UDP_CloseSocket,
	// UDP_Connect,
	// UDP_CheckNewConnections,
	// UDP_Read,
	// UDP_Write,
	// UDP_Broadcast,
	// UDP_AddrToString,
	// UDP_StringToAddr,
	// UDP_GetSocketAddr,
	// UDP_GetNameFromAddr,
	// UDP_GetAddrFromName,
	// UDP_AddrCompare,
	// UDP_GetSocketPort,
	// UDP_SetSocketPort
	// }
	// ,
	{
	"WebSocket",
	false,
	0,
	WebSocket_Init,
	WebSocket_Shutdown,
	WebSocket_Listen,
	WebSocket_OpenSocket,
	WebSocket_CloseSocket,
	WebSocket_Connect,
	WebSocket_CheckNewConnections,
	WebSocket_Read,
	WebSocket_Write,
	WebSocket_Broadcast,
	WebSocket_AddrToString,
	WebSocket_StringToAddr,
	WebSocket_GetSocketAddr,
	WebSocket_GetNameFromAddr,
	WebSocket_GetAddrFromName,
	WebSocket_AddrCompare,
	WebSocket_GetSocketPort,
	WebSocket_SetSocketPort
	}

};

int net_numlandrivers = 1;
