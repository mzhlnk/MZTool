using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualBasic.FileIO;

namespace FP
{
	class Split
	{
		enum PTYPE
		{
			None,
			FPH,
			FPV
		}

		// FileSplit [/H:<start>,<step>,<end>] [/V:<start>,<step>,<end>] <input>[:encoding] <output:###.txt>[:encoding]
		public static int FSplit(string[] args)
		{
			PTYPE Type = PTYPE.None;
			string inFile = null;
			string outHead = null;
			string outMid = null;
			string outEnd = null;
			int iStart = 0;
			int iStep = 1;
			int iEnd = int.MaxValue;
			foreach (string arg in args)
			{
				if (arg.StartsWith("/V", true, System.Globalization.CultureInfo.CurrentCulture) ||
					arg.StartsWith("-V", true, System.Globalization.CultureInfo.CurrentCulture))
				{
					Type = PTYPE.FPV;
					if (arg.Length > 2)
					{
						if (arg[2] != ':') return -1;
						string[] opts = arg.Substring(3).Split(',');
						if (opts.Length > 0 && !int.TryParse(opts[0], out iStep)) return -1;
						if (opts.Length > 1 && !int.TryParse(opts[1], out iStart)) return -1;
						if (opts.Length > 2 && !int.TryParse(opts[2], out iEnd)) return -1;
						if (opts.Length > 3) return -1;
					}
				}
				else if (arg.StartsWith("/H", true, System.Globalization.CultureInfo.CurrentCulture) ||
						 arg.StartsWith("-H", true, System.Globalization.CultureInfo.CurrentCulture))
				{
					Type = PTYPE.FPH;
					if (arg.Length > 2)
					{
						if (arg[2] != ':') return -1;
						string[] opts = arg.Substring(3).Split(',');
						if (opts.Length > 0 && !int.TryParse(opts[0], out iStep)) return -1;
						if (opts.Length > 1 && !int.TryParse(opts[1], out iStart)) return -1;
						if (opts.Length > 2 && !int.TryParse(opts[2], out iEnd)) return -1;
						if (opts.Length > 3) return -1;
					}
				}
				else if (inFile == null)
				{
					inFile = arg;
				}
				else if (outHead == null)
				{
					int idx = arg.LastIndexOf('#');
					if (idx < 0)
					{
						outHead = arg;
						outMid = "{0}";
						outEnd = string.Empty;
					}
					else
					{
						outEnd = arg.Substring(idx + 1);
						int Num = 1;
						for(idx--; idx >= 0; idx--, Num++)
						{
							if (arg[idx] != '#') break;
						}
						outHead = idx >= 0 ? arg.Substring(0, idx + 1) : string.Empty;
						outMid = "{0:d"+ Num + "}";
					}
				}
				else return -1;
			}
			if (outHead == null || iStep < 1 || iStart > iEnd || iStart < 0) return -1;

			try
			{

				if (Type == PTYPE.FPV)
				{
					using (StreamReader sr = Common.GetStreamReader(inFile))
					{
						TextReader tr = sr;
						int ui = 1;
						for (; ui < iStart && !sr.EndOfStream; ui++)
						{
							tr.ReadLine();
						}
						int idx = 1;
						while (ui <= iEnd && !sr.EndOfStream)
						{
							string outFile = outHead + string.Format(outMid, idx++) + outEnd;
							using (TextWriter tw = Common.GetStreamWriter(outFile))
							{
								for (uint i = 0; i < iStep && !sr.EndOfStream; i++)
								{
									tw.WriteLine(tr.ReadLine());
								}
							}
							Console.WriteLine(":=> {0}", outFile);
						}
					}
					return 0;
				}
				else if (Type == PTYPE.FPH)
				{
					using (TextFieldParser parser = Common.GetTextFieldParser(inFile))
					{
						parser.TextFieldType = FieldType.Delimited;
						parser.SetDelimiters(",");
						parser.HasFieldsEnclosedInQuotes = true;
						parser.TrimWhiteSpace = true;

						ArrayList fl = new ArrayList();
						for (int nLines = 0; !parser.EndOfData; nLines++)
						{
							string[] sLines = parser.ReadFields();
							int index = 0;
							int col = 0;
							TextWriter[] tws = (TextWriter[])fl.ToArray(typeof(TextWriter));
							for (; col < sLines.Length && index < tws.Length; col += iStep, index++)
							{
								tws[index].WriteLine(sLines[col]);
							}
							for (int limit = Math.Min(sLines.Length - 1, iEnd); col <= limit; col += iStep, index++)
							{
								string outFile = outHead + string.Format(outMid, col + 1) + outEnd;
								TextWriter tw = Common.GetStreamWriter(outFile);
								for (int i = nLines; i > 0; i--) tw.WriteLine();
								tw.WriteLine(sLines[col]);
								fl.Add(tw);
								Console.WriteLine(":=> {0}", outFile);
							}
							for (; index < tws.Length; index++)
							{
								tws[index].WriteLine();
							}
						}
						foreach (TextWriter tw in fl)
						{
							tw.Close();
						}
						return 0;
					}
				}

				return -1;
			}
			catch (Exception e)
			{
				Console.WriteLine(e.ToString());
				return -2;
			}
		}
	}
}
