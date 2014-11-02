using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.ServiceProcess;

namespace OneWireDomoticzService.Service
{
   [RunInstaller(true)]
// ReSharper disable UnusedMember.Global
   public partial class ServiceRegister : System.Configuration.Install.Installer
// ReSharper restore UnusedMember.Global
   {
      private readonly ServiceProcessInstaller _processInstaller = new ServiceProcessInstaller();
      private readonly ServiceInstaller _serviceInstaller = new ServiceInstaller();

      public ServiceRegister()
      {
         // Le fichier de log doit se retrouver dans un emplacement comme "C:\Windows\SysWOW64"
         Trace.Listeners.Clear();
         Trace.Listeners.Add(new DefaultTraceListener { LogFileName = "OneWireDomoticzServiceInstall.log" });
         Trace.AutoFlush = true;

         InitializeComponent();

         InitServiceInstallers();
      }

      [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
      public override void Install(IDictionary stateSaver)
      {
         Trace.Write("Install...");
         base.Install(stateSaver);
         Trace.WriteLine("OK");

         // Démarrage du service
         Trace.Write("Starting service...");
         var sc = new ServiceController { ServiceName = _serviceInstaller.ServiceName, MachineName = System.Environment.MachineName };
         Trace.Write("Created...");
         sc.Start();
         Trace.WriteLine("OK");
      }

      [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
      public override void Commit(IDictionary savedState)
      {
         Trace.Write("Commit...");
         base.Commit(savedState);
         Trace.WriteLine("OK");
      }

      [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
      public override void Rollback(IDictionary savedState)
      {
         Trace.Write("Rollback...");
         base.Rollback(savedState);
         Trace.WriteLine("OK");
      }

      [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
      public override void Uninstall(IDictionary savedState)
      {
         // Arrêt du service
         var sc = new ServiceController { ServiceName = _serviceInstaller.ServiceName, MachineName = System.Environment.MachineName };
         if (sc.Status == ServiceControllerStatus.Running)
         {
            Trace.Write("Stopping service...");
            sc.Stop();
            Trace.WriteLine("OK");
         }

         Trace.Write("Uninstall...");
         base.Uninstall(savedState);
         Trace.WriteLine("OK");
      }

      private void InitServiceInstallers()
      {
         // Le service tourne sous le compte système
         _processInstaller.Account = ServiceAccount.LocalSystem;

         // Caractéristiques du service
         _serviceInstaller.StartType = ServiceStartMode.Automatic;
         _serviceInstaller.ServiceName = Service.TheServiceName;
         _serviceInstaller.DisplayName = "1-Wire support for Domoticz";
         _serviceInstaller.Description = "1-Wire peripherals access for Domoticz";

         // Ajout des installers
         Installers.Add(_serviceInstaller);
         Installers.Add(_processInstaller);
      }
   }
}
