using System;
using System.Collections.Generic;
using System.Text;

namespace Nettention.Proud
{
	public class PreciseCurrentTime
	{
		private static object timeCritSec = new object();
		static long baseTime = 0;

		public static long GetTimeMs()
		{
			lock (timeCritSec)
			{
				long t = DateTime.Now.Ticks;
				if (baseTime == 0)
				{
					baseTime = t;
				}
				return (t - baseTime) / 10000;
			}
		}
	}
}
