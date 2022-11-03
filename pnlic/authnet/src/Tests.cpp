#include "stdafx.h"
#include "Tests.h"
#include "NetServer.h"

namespace Proud 
{
	void Test_SomeUnitTests( CNetServer* srv )
	{
		((CNetServerImpl*)srv)->TEST_SomeUnitTests();

	}
}