#pragma once 

#include "BufferSegment.h"
#include "../include/AddrPort.h"

namespace Proud
{
#ifdef __MARMALADE__
	int send_segmented_data_without_sengmsg(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags);
	int sendto_segmented_data_without_sendmsg( SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags, const sockaddr* sendTo, int sendToLen );
#endif
}