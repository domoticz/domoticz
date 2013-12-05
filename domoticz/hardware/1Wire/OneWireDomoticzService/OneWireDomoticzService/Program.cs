using System;
using System.Reflection;
using System.ServiceProcess;

namespace OneWireDomoticzService
{
   public static class Program
   {
      /// <summary>
      /// Point d'entrée principal de l'application.
      /// </summary>
      [STAThread]
      private static void Main()
      {
         // Chargement du service
         ServiceBase.Run(new Service.Service());
      }

      public static string Name
      {
         get { return Assembly.GetExecutingAssembly().GetName().Name; }
      }

      public static string Version
      {
         get
         {
            var assembly = Assembly.GetExecutingAssembly();
            return "v" + assembly.GetName().Version.Major + "." + assembly.GetName().Version.Minor + "." + assembly.GetName().Version.Build;
         }
      }
   }
}
