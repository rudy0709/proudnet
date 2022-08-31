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
using System.Linq;
using System.Text;
//using System.Threading.Tasks;

namespace Nettention.Proud
{
	internal class NetServerEvent : Nettention.Proud.INetServerEvent
	{
		NetServer m_owner = null;

		public NetServerEvent(NetServer owner)
			: base()
		{
			m_owner = owner;
		}

		// natvie -> managed callback 되어 수행되는 로직에서 exception 발생시에 managed단에서 try~catch로 예외처리를 해주어야 합니다.
		internal void Internal_HandleException(System.Exception ex)
		{
			// exceptionHandler 등록되어 있지 않으면, 무시합니다. networker thread or user worker thread의 루프가 중단되어 버리는 문제를 피하기 위해서 입니다.
			if (m_owner.ExceptionHandler != null)
			{
				m_owner.ExceptionHandler(ex);
			}
		}

		// INetCoreEvent
		public override void OnError(Nettention.Proud.ErrorInfo errorInfo)
		{
			try
			{
				if (m_owner.ErrorHandler != null)
				{
					m_owner.ErrorHandler(errorInfo);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}

		}

		public override void OnWarning(Nettention.Proud.ErrorInfo errorInfo)
		{
			try
			{
				if (m_owner.WarningHandler != null)
				{
					m_owner.WarningHandler(errorInfo);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnInformation(Nettention.Proud.ErrorInfo errorInfo)
		{
			try
			{
				if (m_owner.InformationHandler != null)
				{
					m_owner.InformationHandler(errorInfo);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnException(Nettention.Proud.NativeException e)
		{
			if (m_owner.ExceptionHandler != null)
			{
				m_owner.ExceptionHandler(new System.Exception(e.what()));
			}
		}

		public override void OnNoRmiProcessed(ushort rmiID)
		{
			try
			{
				if (m_owner.NoRmiProcessedHandler != null)
				{
					m_owner.NoRmiProcessedHandler((Proud.RmiID)rmiID);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}

		}

		public override void OnReceiveUserMessage(Nettention.Proud.HostID sender, Nettention.Proud.NativeRmiContext rmiContext, global::System.IntPtr payload, int payloadLength)
		{
			try
			{
				if (m_owner.ReceiveUserMessageHandler != null)
				{
					ByteArray managedPayload = ConvertToManaged.NativeDataToManagedByteArray(payload, payloadLength);

					RmiContext context = new RmiContext();
					context.compressMode = rmiContext.compressMode;
					context.encryptMode = rmiContext.encryptMode;
					context.maxDirectP2PMulticastCount = rmiContext.maxDirectP2PMulticastCount;
					context.relayed = rmiContext.relayed;
					context.reliability = rmiContext.reliability;
					context.sentFrom = rmiContext.sentFrom;
					context.uniqueID = rmiContext.uniqueID;

					m_owner.ReceiveUserMessageHandler((Proud.HostID)sender, context, managedPayload);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnTick(global::System.IntPtr context)
		{
			try
			{
				if (m_owner.TickHandler != null)
				{
					m_owner.TickHandler((object)context);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnUserWorkerThreadCallbackBegin(UserWorkerThreadCallbackContext ctx)
		{

		}

		public override void OnUserWorkerThreadCallbackEnd(UserWorkerThreadCallbackContext ctx)
		{
		}

		// INetServerEvent
		public override void OnClientJoin(Nettention.Proud.NetClientInfo clientInfo)
		{
			try
			{
				if (m_owner.ClientJoinHandler != null)
				{
					m_owner.ClientJoinHandler(clientInfo);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnClientLeave(Nettention.Proud.NetClientInfo clientInfo, Nettention.Proud.ErrorInfo errorinfo, Nettention.Proud.NativeByteArray comment)
		{
			try
			{
				if (m_owner.ClientLeaveHandler != null)
				{
					m_owner.ClientLeaveHandler(clientInfo, errorinfo, ConvertToManaged.NativeByteArrayToManagedByteArray(comment));
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnClientOffline(Nettention.Proud.RemoteOfflineEventArgs args)
		{
			try
			{
				if (m_owner.ClientOfflineHandler != null)
				{
					m_owner.ClientOfflineHandler(args);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnClientOnline(Nettention.Proud.RemoteOnlineEventArgs args)
		{
			try
			{
				if (m_owner.ClientOnlineHandler != null)
				{
					m_owner.ClientOnlineHandler(args);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override bool OnConnectionRequest(Nettention.Proud.AddrPort clientAddr, Nettention.Proud.NativeByteArray userDataFromClient, Nettention.Proud.NativeByteArray reply)
		{
			try
			{
				if (m_owner.ConnectionRequestHandler != null)
				{
					return m_owner.ConnectionRequestHandler(clientAddr,
						ConvertToManaged.NativeByteArrayToManagedByteArray(userDataFromClient),
						ConvertToManaged.NativeByteArrayToManagedByteArray(reply)
						);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}

			return true;
		}

		public override void OnP2PGroupJoinMemberAckComplete(Nettention.Proud.HostID groupHostID, Nettention.Proud.HostID memberHostID, Nettention.Proud.ErrorType result)
		{
			try
			{
				if (m_owner.P2PGroupJoinMemberAckCompleteHandler != null)
				{
					m_owner.P2PGroupJoinMemberAckCompleteHandler((Proud.HostID)groupHostID, (Proud.HostID)memberHostID, result);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnUserWorkerThreadBegin()
		{
			try
			{
				if (m_owner.UserWorkerThreadBeginHandler != null)
				{
					m_owner.UserWorkerThreadBeginHandler();
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnUserWorkerThreadEnd()
		{
			try
			{
				if (m_owner.UserWorkerThreadEndHandler != null)
				{
					m_owner.UserWorkerThreadEndHandler();
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnClientHackSuspected(Nettention.Proud.HostID clientID, Nettention.Proud.HackType hackType)
		{
			try
			{
				if (m_owner.ClientHackSuspectedHandler != null)
				{
					m_owner.ClientHackSuspectedHandler((Proud.HostID)clientID, hackType);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}

		public override void OnP2PGroupRemoved(Nettention.Proud.HostID groupID)
		{
			try
			{
				if (m_owner.P2PGroupRemovedHandler != null)
				{
					m_owner.P2PGroupRemovedHandler((Proud.HostID)groupID);
				}
			}
			catch (System.Exception ex)
			{
				Internal_HandleException(ex);
			}
		}
	}
}
