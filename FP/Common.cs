using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using Microsoft.VisualBasic.FileIO;

namespace FP
{
	class Common
	{
		public static Encoding ParseFileName(ref string filename)
		{
			Encoding enc = Encoding.Default;
			int x = filename.LastIndexOf(':');
			if (x > 0 && (x + 1) < filename.Length && filename[x + 1] != '\\')
			{
				enc = Encoding.GetEncoding(filename.Substring(x + 1));
				filename = filename.Substring(0, x);
			}
			return enc;
		}

		public static StreamReader GetStreamReader(string filename)
		{
			Encoding enc = ParseFileName(ref filename);
			return new StreamReader(filename, enc);
		}
		public static StreamWriter GetStreamWriter(string filename, bool bAppend = true)
		{
			Encoding enc = ParseFileName(ref filename);
			return new StreamWriter(filename, bAppend, enc);
		}
		public static TextFieldParser GetTextFieldParser(string filename)
		{
			Encoding enc = ParseFileName(ref filename);
			return new TextFieldParser(filename, enc);
		}
	}
}
