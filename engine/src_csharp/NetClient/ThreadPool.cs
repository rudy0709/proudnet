/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/


using System;
using System.Collections.Generic;
using System.Text;

namespace Nettention.Proud
{
	/**
	\~korean
	서버사이드 호스트 모듈(CNetServer, CNetClient)을 위한 thread pool 객체입니다.

	자세한 것은 \ref thread_pool_sharing 을 참고하십시오.

	\~english
	This is thread pool object for server-sided host module(CNetServer, CNetClient)

	Please refer to \ref thread_pool_sharing for more detail.

	\~chinese
	为了server-sided 主机模型（CNetServer, CNetClient）的thread pool对象。

	详细请参考\ref thread_pool_sharing%。

	\~japanese
	\~

	사용법
	- tp = CThreadPool.Create(threadCount);
	- NC,NS에서 tp를 변수로 참조

	TODO: 상기 주석 다듬은 후 다국어화해야!

	*/
	public class ThreadPool : IDisposable
	{
		// SWIG에서 만들어진 C++ ThreadPool 래핑 클래스.
		private Proud.NativeThreadPool m_native_ThreadPool = null;

		private bool m_disposed = false;

		public ThreadPool(int threadCount)
		{
			try
			{
				m_native_ThreadPool = NativeThreadPool.Create(threadCount);
			}
			catch (System.TypeInitializationException ex)
			{
				// c++ ProudNetServerPlugin.dll, ProudNetClientPlugin.dll 파일이 작업 경로에 없을 때
				throw new System.Exception(ClientNativeExceptionString.TypeInitializationExceptionString);
			}
		}

		~ThreadPool()
		{
			Dispose(false);
		}

		/**
		\~korean
		사용자가 명시적으로 호출하지 않도록 합니다.
		IDisposable 인터페이스 구현
		\~english
		*/
		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (m_disposed)
				return;

			if (disposing)
			{
			}

			m_disposed = true;
		}

		public NativeThreadPool GetNativeThreadPool()
		{
			return m_native_ThreadPool;
		}

		/**
		\~korean
		원하는 수의 스레드로 갯수를 조절한다.
		0~512까지 가능.
		이 함수는 즉시 리턴되고, 시간이 지나면서 목표치로 가게 된다.
		0이 될 경우 사용자는 CThreadPool.Process를 매번 호출해 주어야 한다.
		\~english
		*/
		public void SetDesiredThreadCount(int threadCount)
		{
			if (m_native_ThreadPool != null)
			{
				m_native_ThreadPool.SetDesiredThreadCount(threadCount);
			}
		}

		/**
		\~korean
		zero thread pool을 위한, heartbeat 처리를 한다.
		zero thread pool인 경우 지속적으로 호출해야 한다.
		인자: 최대 대기 시간. 0을 넣으면 무대기 실행.
		\~english
		*/
		public void Process(int timeoutMs)
		{
			if (m_native_ThreadPool != null)
			{
				m_native_ThreadPool.Process(timeoutMs);
			}
		}

	}
}
