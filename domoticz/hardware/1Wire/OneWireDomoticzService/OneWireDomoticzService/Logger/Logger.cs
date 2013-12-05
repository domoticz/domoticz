using System;
using System.Diagnostics;
using System.IO;

namespace OneWireDomoticzService.Logger
{
   public static class Logger
   {
      static Logger()
      {
         Trace.Listeners.Add(new RollingTraceListener(Path.Combine(Directory.GetCurrentDirectory(),Program.Name+".log")));
         Trace.AutoFlush = true;
      }

      public static void Log(string format, params object[] args)
      {
         Log(string.Format(format, args));
      }

      public static void Log(string str)
      {
         Trace.WriteLine(DateTime.Now + " : " + str);
      }
   }
}
