using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace FP
{
	class Plus
	{
		abstract class STVtr
		{
			public abstract string ReadLine();

			public virtual void Open() { }
			public virtual void Close() { }
		}
		class TextVtr : STVtr
		{
			public string Text { get; set; }
			public TextVtr(string text)
			{ Text = text; }

			public override string ReadLine()
			{ return Text; }
		}
		class StreamVtr : TextVtr
		{
			public StreamReader reader = null;
			public StreamVtr(string text)
				: base(text)
			{ }

			static int _count = 0;
			public static bool HasStream { get { return _count > 0; } }
			public override void Open()
			{
				Close();
				reader = Common.GetStreamReader(Text);
				_count++;
				if (reader.EndOfStream)
					Close();
			}
			public override void Close()
			{
				if (reader != null)
				{
					reader.Close();
					reader = null;
					_count--;
				}
			}
			public override string ReadLine()
			{
				if (reader == null)
					return null;
				string text = reader.ReadLine();
				if (reader.EndOfStream) Close();
				return text;
			}
		}

		static STVtr[] GetVtr(string[] args)
		{
			STVtr[] stVtr = null;
			{
				ArrayList srcArray = new ArrayList();
				string[] THEAD = new string[] { "/T:", "-T:" };
				int idx = 0;
				foreach (string val in args)
				{
					if (val == "+")
					{
						idx--;
					}
					else if (idx == 0)
					{
						idx++;
						if (val.Length >= 3 && THEAD.Contains(val.Substring(0, 3).ToUpper()))
						{
							srcArray.Add(new TextVtr(val.Substring(3)));
						}
						else
						{
							if (string.IsNullOrEmpty(val))
							{
								return null;
							}
							srcArray.Add(new StreamVtr(val));
						}
					}
					else
					{
						return null;
					}
				}
				if (idx < 0)
					return null;

				try
				{
					stVtr = (STVtr[])srcArray.ToArray(typeof(STVtr));
					foreach (STVtr vtr in stVtr)
					{
						vtr.Open();
					}
				}
				catch
				{
					foreach (STVtr vtr in stVtr)
					{
						try
						{
							vtr.Close();
						}
						catch { }
					}
					throw;
				}
			}
			return stVtr;
		}
		
		public static int FHPlus(string[] args)
		{
			try
			{
				STVtr[] stVtr = GetVtr(args);
				if (stVtr == null) return -1;
				{
					while (StreamVtr.HasStream)
					{
						foreach (STVtr vtr in stVtr)
						{
							string text = vtr.ReadLine();
							if (!string.IsNullOrEmpty(text))
								Console.Write(text);
						}
						Console.WriteLine();
					}
				}

				return 0;
			}
			catch (Exception e)
			{
				Console.Error.WriteLine(e.ToString());
				return -2;
			}
		}

		public static int FVPlus(string[] args)
		{
			try
			{
				STVtr[] stVtr = GetVtr(args);
				if (stVtr == null) return -1;
				{
					foreach (STVtr vtr in stVtr)
					{
						StreamVtr sv = vtr as StreamVtr;
						if (sv != null)
						{
							StreamReader sr = sv.reader;
							while (!sr.EndOfStream)
							{
								Console.WriteLine(sr.ReadLine());
							}
						}
						else
						{
							Console.WriteLine(vtr.ReadLine());
						}
					}
				}

				return 0;
			}
			catch (Exception e)
			{
				Console.Error.WriteLine(e.ToString());
				return -1;
			}
		}

		public static int FPlus(string[] args)
		{
			if (args.Length > 0)
			{
				if (args[0].StartsWith("-") || args[0].StartsWith("/"))
				{
					switch (args[0].Substring(1).ToUpper())
					{
						case "H":
							return FHPlus(args.Skip(1).ToArray());
						case "V":
							return FVPlus(args.Skip(1).ToArray());
					}
				}
			}
			return -1;
		}
	}

}
