#include "stdafx.h"
#include "SendMessage.h"
#include "SocketUtil.h"
 
namespace Proud 
{
#ifdef __MARMALADE__
	// WSASend의 segment buffer를 송신하는 것을 흉내낸다. 비 윈도를 위함.
	// https://www.madewithmarmalade.com/ko/node/55990 workaround
	// 내부적으로 여러번의 sendto()가 호출된다. 이 함수의 리턴값은 여러차례의 send()를 호출하면서 누계된 총 크기를 리턴한다. 중간에 오류가 나는 경우(가령 소켓을 중간에 닫는 경우)라도, 여지껏 성공한 것들이
	// 있으므로, 여지껏 성공한 것들에 대한 양수를 리턴한다.
	// 하지만 내부적으로 소켓 상태는 메롱일 수도 있고 아닐 수도(EINTR,EAGAIN,EWOULDBLOCK) 있는데, 여하건 다음 턴에서 이 함수는 첫빠따로 바로 <0을 리턴할테니 그때 처리하면 된다.
	// Caller에서 Intr 에러를 처리한다.
	int send_segmented_data_without_sengmsg(SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags)
	{
		// 비 win32에서는 WSASendTo처럼 segmented buffer를 사용할 수 없다.
		// 따라서 불가피하게 이들을 조립한 사본을 만들어야 한다.
		// 어차피 입력받은 sendBuffer는 header나 payload들을 가리키는데, payload가 있는 곳에 직접 assemble을 할 수는 없고,
		// TCP는 stream oriented이므로 각 segment에 대해서 send를 호출해도 되지만, 이렇게 하면 지나치게 많은 syscall부하가 생긴다.
		// 그렇다고, 전부다 조립하면, TCP stream에 들어있는 양이 100KB이상 쌓여있는 경우, 지나친 복사 부하가 생긴다. (UDP는 이럴 일은 없다.)
		// 따라서 적정 선으로 일부는 조립하고 일부는 여러번의 syscall로 적절히 섞는다.

		int segmentCursor = 0; // 현재 처리중인 segment 인덱스
		int totalSegmentCount = sendBuffer.Array().GetCount(); // segment의 갯수 (총 바이트 크기가 아님!)
		int completedLength = 0; // 여러번의 send()를 호출하면서 socket send buffer에 누적한 총량

		while(1)
		{
			// 종료 조건인지? (사용자가 빈 것을 보내는 경우 때문)
			if(segmentCursor >= totalSegmentCount)
				return completedLength;

			// 먼저, 작은 segment들을 조립한다. 조립된 것에 대해 1회의 syscall을 한다.
			int usedSegmentCount = 0; 
			ByteArrayPtr assembled = sendBuffer.AssembleUpTo(1024, segmentCursor, &usedSegmentCount);

			if(usedSegmentCount > 0) // 조립된 것이 있으면
			{
				// 조립된 것에 대한 1회의 syscall
				int r = ::send(socket, (const char*)assembled.GetData(), assembled.GetCount(), flags);

				/* non-blocked or blocked send를 하는 경우 socket 내 send buffer안에 전부 혹은 일부를 채운다.
				따라서 0 이하가 나오면 에러 혹은 더 이상 넣을 공간이 없음을 의미하므로 끝.

				Q: EINTR이 리턴되면 재시도하는 로직이 있어야 할텐데요?
				A: 없어도 됩니다. 만약 위에서 EINTR이 리턴되면 
				   아래에서는 최근까지 성공했던 send bytes를 리턴해주면 됩니다. 
				   그러면 콜러는 따로 errno를 체크 안합니다.
				   한번도 성공 못했으면 -1을 리턴할테고 그때는 caller가 다로 체크하겠고
				   그때 EINTR이면 알아서 체크합니다.
				   EINTR 발생시 재시도는 FastSocket level에서 하는 것이 원칙적이니까요.
				*/
				if(r <= 0)
				{
					if(completedLength <= 0)
						return r;
					else
						return completedLength; // 과거에 성공한 것을 인정해주는 것이 우선. 콜러가 가진 송신큐에서의 처리가 필요하니까.
				}

				// 0 초과이면 송신큐에 쌓았음을 의미한다. 누적하자.
				// 여기서 리턴값은 위 send()의 리턴값이 아닌, 앞 루프에서 send가 성공했던 누계를 리턴해야 한다.
				completedLength += r;

				// 전부 다 socket 송신큐에 넣었다면 다음 것도 계속 수행해야 하지만, 일부만 성공적으로 들어간 경우 여기서 멈추도록 하자.
				if(r < assembled.GetCount())
				{
					return completedLength;
				}

				// 조립에 사용된 것들은 건너뛰어야
				segmentCursor += usedSegmentCount;
			}

			// 종료 조건인지? 
			if(segmentCursor >= totalSegmentCount)
				return completedLength;

			// 조립된 segment들을 처리하고 남은 것은 매우 큰 segment일 것이다. 그것을 처리하도록 한다.
			{
				void* buf = sendBuffer.Array()[segmentCursor].buf;
				int len = sendBuffer.Array()[segmentCursor].len;
				int r = ::send(socket, (const char*)buf, len, flags);

				// non-blocked or blocked send를 하는 경우 socket 내 send buffer안에 전부 혹은 일부를 채운다.
				// 따라서 0 이하가 나오면 에러 혹은 더 이상 넣을 공간이 없음을 의미하므로 끝.
				if(r <= 0)
				{
					if(completedLength <= 0)
						return r;
					else
						return completedLength; // 과거에 성공한 것을 인정해주는 것이 우선. 콜러가 가진 송신큐에서의 처리가 필요하니까.
				}

				// 0 초과이면 송신큐에 쌓았음을 의미한다. 누적하자.
				completedLength += r;

				// 전부 다 socket 송신큐에 넣었다면 다음 것도 계속 수행해야 하지만, 그렇지 않다면 여기서 멈추도록 하자.
				if(r < len)
				{
					return completedLength;
				}

				// 1회의 syscall을 했으므로, 이것에 대한 1개의 segment는 건너뛰어야
				segmentCursor++;
			}
		}
	}

	// WSASend의 segment buffer를 송신하는 것을 흉내낸다.
	// https://www.madewithmarmalade.com/ko/node/55990 workaround
	// Caller에서 Intr 에러를 처리한다.
	int sendto_segmented_data_without_sendmsg( SOCKET socket, CFragmentedBuffer& sendBuffer, unsigned int flags, const sockaddr* sendTo, int sendToLen )
	{
	/* 비 win32에서는 WSASendTo처럼 segmented buffer를 사용할 수 없다.
	따라서 불가피하게 이들을 조립한 사본을 만들어야 한다.
	어차피 입력받은 sendBuffer는 header나 payload들을 가리키는데, payload가 있는 곳에 직접 assemble을 할 수는 없고,
	복사는 해야 한다. 리눅스가 뭐 그렇지.
	UDP는 답이 없다. 전부다 조립해야 한다. TCP처럼 stream oriented가 아니니까. 
	
	Q: sendto(MSG_MORE)와 sendto(MSG_CORK)를 쓰면 되지 않나요?
	A: sendBuffer의 각 segment 횟수만큼 sendto syscall을 하게 됩니다. 즉 syscall 횟수가 늘어납니다. 더 나쁘죠. */
		ByteArrayPtr assembled = sendBuffer.Assemble();
		return ::sendto(socket, (const char*)assembled.GetData(), (size_t)assembled.GetCount(), flags,
			sendTo, sendToLen);
	}
#endif


}