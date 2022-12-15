#pragma once 

#include "../include/FakeClr.h"
#include "SendFragRefs.h"
#include "ReliableUDPFrame.h"

namespace Proud 
{

	class CRemotePeerReliableUdpHelper
	{
	public:
		 PROUD_API static int GetRandomFrameNumber(Random &random, bool simpleProtocolMode);

		static void BuildSendDataFromFrame( ReliableUdpFrame &frame, CSendFragRefs &ret, CMessage& header );
		static void BuildRelayed2LongDataFrame( int frameNumber, const CSendFragRefs &content, ReliableUdpFrame &ret );

	};

}
