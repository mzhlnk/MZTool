using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace FEC
{
	class Program
	{
		// FEC <FIN>:ENCODING <FOUT>:ENCODING
		static int Main(string[] args)
		{
			if(args.Length != 2)
			{
				Console.WriteLine("FEC <FIN>:ENCODING <FOUT>:ENCODING");
				return -1;
			}

			try
			{
				string inFile = args[0];
				string outFile = args[1];
				Encoding inec = null;
				Encoding outec = null;
				int x = inFile.LastIndexOf(':');
				if (x > 0 && (x + 1) < inFile.Length && inFile[x + 1] != '\\')
				{
					inec = Encoding.GetEncoding(inFile.Substring(x + 1));
					inFile = inFile.Substring(0, x);
				}
				x = outFile.LastIndexOf(':');
				if (x > 0 && (x + 1) < outFile.Length && outFile[x + 1] != '\\')
				{
					outec = Encoding.GetEncoding(outFile.Substring(x + 1));
					outFile = outFile.Substring(0, x);
				}

				using (StreamReader sr = inec != null ? new StreamReader(inFile, inec) : new StreamReader(inFile))
				{
					using (StreamWriter sw = outec != null ? new StreamWriter(outFile, true, outec) : new  StreamWriter(outFile, true))
					{
						while (!sr.EndOfStream)
							sw.WriteLine(sr.ReadLine());
					}
				}

				return 0;
			}
			catch(Exception e)
			{
				Console.WriteLine(e.ToString());
			}
			return -2;
		}
	}
}
