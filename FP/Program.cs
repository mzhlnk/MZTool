using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace FP
{
	class Program
	{
		// fp -s -h|v[:step:1][,start:0][,end] <input> <output>
		// fp -p -h|v [file] + [/T:[Text]]...
		static int Main(string[] args)
		{
			if (args.Length > 0)
			{
				if (args[0].StartsWith("-") || args[0].StartsWith("/"))
				{
					int ret = -1;
					switch (args[0].Substring(1).ToUpper())
					{
						case "S":
							ret = Split.FSplit(args.Skip(1).ToArray());
							break;
						case "P":
							ret = Plus.FPlus(args.Skip(1).ToArray());
							break;
						case "R":
							break;
					}
					if (ret != -1) return ret;
				}
			}

			Console.WriteLine("FP /P /H|V [file] + [/T:[Text]]...");
			Console.WriteLine("FP /S /H|V[:step:1][,start:0][,end] <input> <output>");
			//Console.WriteLine("FP /R /P:<old>=[new] <input> <output>");
			return -1;
		}
	}
}
