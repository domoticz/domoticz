using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace OneWireDomoticzService.Logger
{
   public static class Logger
   {
      static Logger()
      {
         var dir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
         if (dir == null)
             dir = Directory.GetCurrentDirectory();

         Trace.Listeners.Add(new RollingTraceListener(Path.Combine(dir,Program.Name+".log")));
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
