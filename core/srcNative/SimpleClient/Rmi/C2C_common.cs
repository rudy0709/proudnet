     
using System;
 
     	     

namespace C2C{
internal class Common
	{
		// Message ID that replies to each RMI method. 
	
public const Nettention.Proud.RmiID P2PChat = (Nettention.Proud.RmiID)2000+1;

public const Nettention.Proud.RmiID P2PChat2 = (Nettention.Proud.RmiID)2000+2;

 

		// List that has RMI ID.
		public static Nettention.Proud.RmiID[] RmiIDList = new Nettention.Proud.RmiID[] {
P2PChat,
P2PChat2,
};
	}
}
