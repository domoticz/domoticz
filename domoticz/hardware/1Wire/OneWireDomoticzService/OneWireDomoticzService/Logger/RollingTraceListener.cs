using System;
using System.Diagnostics;
using System.IO;

namespace OneWireDomoticzService.Logger
{
   /// <summary>
   /// author: pedroliska.com
   /// creation-date: 2009-05-01
   /// 
   /// This class handles writing to a log file and rolling it based on 
   /// the file size. Currently you only need to send one parameter to the
   /// constructor: the logFullFilePath
   /// 
   /// From the logFullFilePath, this class will determine the folder location where
   /// to place all logs and the file names for the log files. Log files will be
   /// rolled when the file hits 5MB; this will be customizable in the future via
   /// constructor parameters or public properties.
   /// 
   /// For a logFullFilePath like this one: C:\dir1\dir2\log-file-name.log
   /// 
   /// All of the log files will be located in C:\dir1\dir2 , the most recent
   /// log file will always be log-file-name.log and once log-file-name.log hits 5MB, 
   /// it will be renamed log-file-name-01.log and a new log-file-name.log file will 
   /// be created. Then when log-file-name.log hits 5MB again, log-file-name-01.log 
   /// will become log-file-name-02.log, log-file-name.log will become log-file-name-01.log 
   /// and a new log-file-name.log file will be created. And so on...
   /// 
   /// Currently you will have 15 log-file-name-NN.log files; but this will be 
   /// customizeable in the future via constructor parameters or public properties.
   /// </summary>
   public class RollingTraceListener : TraceListener
   {
      // The amount of numbered file logs
      private const int NumberedFileMax = 5;
      private const int MaxFileSize = 10 * 1024 * 1024;

      private readonly string _folder;
      private readonly string _fileName;
      private readonly string _extension;

      public RollingTraceListener(string logFullPath)
      {
         _folder = Path.GetDirectoryName(logFullPath);
         _fileName = Path.GetFileNameWithoutExtension(logFullPath);
         _extension = Path.GetExtension(logFullPath);
      }

      private string LogFullPath
      {
         get { return Path.Combine(_folder, _fileName + _extension); }
      }

      private string GetLogFullPath(int number)
      {
         return Path.Combine(_folder, _fileName + "_" + number + _extension);
      }

      private void LogMessage(string message)
      {
         var logFile = LogFullPath;

         if (!File.Exists(logFile))
         {
            // Le fichier n'existe pas, création de son répertoire si nécessaire
            if (!string.IsNullOrEmpty(_folder) && !Directory.Exists(_folder))
               Directory.CreateDirectory(_folder);
         }
         else
         {
            // Le fichier existe, a-t-il atteint sa limite de taille ?
            if (new FileInfo(logFile).Length > MaxFileSize)
            {
               // Le fichier est plein, nous devons tourner les fichiers
               RollFiles();
            }
         }

         // Ecriture du message
         using (var writer = new StreamWriter(LogFullPath, true))
            writer.Write(message);
      }

      /// <summary>
      /// Basculement des fichiers de log :
      /// - Fichier_3 est supprimé
      /// - Fichier_2 est renommé en Fichier_3
      /// - Fichier_1 est renommé en Fichier_2
      /// - Fichier est renommé en Fichier_1
      /// </summary>
      private void RollFiles()
      {
         string currentLogFullPath = null;
         string previousLogFullPath = null;
         for (var i = NumberedFileMax - 1; i > 0; i--)
         {
            currentLogFullPath = GetLogFullPath(i);

            if (File.Exists(currentLogFullPath))
            {
               if (previousLogFullPath == null)
                  previousLogFullPath = GetLogFullPath(i + 1);

               // La méthode File.Copy permet d'écraser le fichier (contrairement à File.Move)
               File.Copy(currentLogFullPath, previousLogFullPath, true);
            }

            previousLogFullPath = currentLogFullPath;
         }
         if (!string.IsNullOrEmpty(currentLogFullPath))
            File.Copy(LogFullPath, currentLogFullPath, true);

         File.Delete(LogFullPath);
      }

      public override void Write(string message)
      {
         LogMessage(message);
      }

      public override void WriteLine(string message)
      {
         LogMessage(message + Environment.NewLine);
      }
   }
}
