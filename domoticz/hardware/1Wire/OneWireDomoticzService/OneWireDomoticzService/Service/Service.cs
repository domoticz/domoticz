using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.ServiceProcess;
using System.Text;
using System.Threading;
using Newtonsoft.Json;
using com.dalsemi.onewire;
using com.dalsemi.onewire.adapter;
using com.dalsemi.onewire.container;

namespace OneWireDomoticzService.Service
{
   class Service : ServiceBase
   {
      private const int OneWireServicePort = 1664;
      private Thread _thread;
      private Socket _serverSocket;
      private Socket _clientSocket;
      private Dictionary<string, Device> _devices;

      public Service()
      {
         AutoLog = false;
         ServiceName = TheServiceName;
         CanPauseAndContinue = false;
      }

      protected override void OnStart(string[] args)
      {
         Logger.Logger.Log("Service start");

         _thread = new Thread(ThreadDoWork);
         _thread.Start();
      }

      protected override void OnStop()
      {
         // Let time to clean stop the service
         RequestAdditionalTime(10000);

         // Let chance to end clean the thread (send/receive pending data)
         Thread.Sleep(100);

         // Socket usage is blocking, disposing sockets will resume the thread
         if (_clientSocket != null)
            _clientSocket.Close();
         if (_serverSocket != null)
            _serverSocket.Close();

         _thread.Join();

         // Indicate a successful exit.
         ExitCode = 0;

         Logger.Logger.Log("Service stopped");
      }

      private void ThreadDoWork()
      {
         // Version log
         Logger.Logger.Log("***** " + Program.Name + " " + Program.Version + " *****");

         // Start socket server
         Logger.Logger.Log("StartServer");
         StartServer();
         if (_serverSocket == null)
            return;

         try
         {
            while (true)
            {
               // Wait for a client...
               Logger.Logger.Log("Server ready. Wait for client connexion...");
               _clientSocket = _serverSocket.Accept();

               // Client connected, wait for a message
               Logger.Logger.Log("Client connected");
               while (_clientSocket != null)
               {
                  try
                  {
                     Logger.Logger.Log("Wait for request...");
                     var request = WaitForRequest();
                     if (request == null)
                     {
                        // Client is disconnecting
                        _clientSocket.Close();
                        _clientSocket = null;
                        break;
                     }

                     ProcessRequest(request);
                  }
                  catch (SocketException exception)
                  {
                     if (exception.SocketErrorCode == SocketError.ConnectionAborted ||
                        exception.SocketErrorCode == SocketError.ConnectionReset)
                     {
                        // Connexion reset by peer
                        if (_clientSocket != null) _clientSocket.Close();
                        _clientSocket = null;
                     }
                     else
                     {
                        Logger.Logger.Log("Exception received : " + exception);
                     }
                  }
                  catch (ObjectDisposedException)
                  {
                     // Socket is disposed when service is stopping
                     _clientSocket = null;
                  }
                  catch (Exception exception)
                  {
                     Logger.Logger.Log("Exception received : " + exception);
                  }
               }
               Logger.Logger.Log("Client disconnected");
            }
         }
         catch (SocketException exception)
         {
            if (exception.SocketErrorCode != SocketError.Interrupted)
               Logger.Logger.Log("Exception received : " + exception);
         }
         catch (Exception exception)
         {
            Logger.Logger.Log("Exception received : " + exception);
         }
         _clientSocket = null;

         // Needed if the stop is not explicitly called (by OnStop)
         var sc = new ServiceController { ServiceName = ServiceName, MachineName = Environment.MachineName };
         if (sc.Status != ServiceControllerStatus.StopPending)
         {
            Logger.Logger.Log("Service stop");
            Environment.Exit(1);
         }
      }

      private DSPortAdapter _oneWireAdapter;
      private DSPortAdapter OneWireAdapter
      {
         get
         {
            if (_oneWireAdapter != null)
               return _oneWireAdapter;

            try
            {
               _oneWireAdapter = OneWireAccessProvider.getDefaultAdapter();
            }
            catch (OneWireIOException exception)
            {
               Logger.Logger.Log("Communication with 1-wire adapter error : " + exception);
            }
            catch (OneWireException exception)
            {
               Logger.Logger.Log("No 1-wire adapter found : " + exception);
            }
            return _oneWireAdapter;
         }
      }

      // Disable warning CS0649:
      // "Field '...' is never assigned to, and will always have its default value null"
#pragma warning disable 649
      internal class Request
      {
         public object IsAvailable;
         public object GetDevices;
         public ReadDataRequest ReadData;
         public ReadNbChanelsRequest ReadChanelsNb;
         public WriteDataRequest WriteData;

         public class ReadDataRequest
         {
            public string Id;
            public UInt32 Unit;
         }

         public class ReadNbChanelsRequest
         {
            public string Id;
         }

         public class WriteDataRequest
         {
            public string Id;
            public UInt32 Unit;
            public bool Value;
         }
      }
#pragma warning restore 649

      private string WaitForRequest()
      {
         var requestSize = new byte[sizeof(UInt32)];
         if (_clientSocket.Receive(requestSize) == 0)
            return null;

         var request = new byte[BitConverter.ToUInt32(requestSize, 0)];
         if (_clientSocket.Receive(request) == 0)
            return null;

         return Encoding.ASCII.GetString(request);
      }

      private void ProcessRequest(string jsonRequest)
      {
         Logger.Logger.Log("Request received <== " + jsonRequest.TrimEnd('\n'));

         Request request = JsonConvert.DeserializeObject<Request>(jsonRequest);

         string jsonAnswer;
         try
         {
            if (OneWireAdapter == null)
               jsonAnswer = JsonConvert.SerializeObject(new AvailableAnswer { Available = false });

            else
            {
               if (request.IsAvailable != null)
                  jsonAnswer = ProcessIsAvailableRequest();
               else if (request.GetDevices != null)
                  jsonAnswer = ProcessGetDevicesRequest();
               else if (request.ReadData != null)
                  jsonAnswer = ProcessReadDataRequest(request.ReadData);
               else if (request.ReadData != null)
                  jsonAnswer = ProcessReadChanelsNbRequest(request.ReadChanelsNb);
               else if (request.WriteData != null)
                  jsonAnswer = ProcessWriteDataRequest(request.WriteData);
               else
                  jsonAnswer = JsonConvert.SerializeObject(new InvalidRequestAnswer { InvalidRequestReason = "UnknownRequestType" });
            }
         }
         catch (OneWireIOException exception)
         {
            Logger.Logger.Log("Communication with 1-wire adapter error : " + exception);
            jsonAnswer = JsonConvert.SerializeObject(new AvailableAnswer { Available = false });
         }
         catch (OneWireException exception)
         {
            Logger.Logger.Log("No 1-wire adapter found : " + exception);
            jsonAnswer = JsonConvert.SerializeObject(new AvailableAnswer { Available = false });
         }

         Logger.Logger.Log("Answer send ==> " + jsonAnswer);
         var frame = Encoding.ASCII.GetBytes(jsonAnswer);
         _clientSocket.Send(BitConverter.GetBytes(frame.Length));
         _clientSocket.Send(frame);
      }

      #region Invalid request answer
      internal class InvalidRequestAnswer { public string InvalidRequestReason; }
      #endregion

      #region IsAvailable request
      internal class AvailableAnswer { public bool Available; }
      private string ProcessIsAvailableRequest()
      {
         var answer = new AvailableAnswer { Available = OneWireAdapter != null };
         return JsonConvert.SerializeObject(answer);
      }
      #endregion

      #region Get all devices request
      public class Device
      {
         public UInt32 Family;
         public string Id;
         [JsonIgnore]
         public string FullId;
      }
      public class GetDevicesAnswer
      {
         public List<Device> Devices = new List<Device>();
      }
      private string ProcessGetDevicesRequest()
      {
         // Search for all 1-wire devices
         OneWireAdapter.beginExclusive(true);
         var devices = OneWireAdapter.getAllDeviceContainers();
         OneWireAdapter.endExclusive();

         // Rebuild internal devices list
         _devices = new Dictionary<string, Device>();
         while (devices.hasMoreElements())
         {
            var dev = ConvertDevice((OneWireContainer)devices.nextElement());
            _devices[dev.Id] = dev;
         }

         // Build answer
         var answer = new GetDevicesAnswer { Devices = new List<Device>(_devices.Values) };

         return JsonConvert.SerializeObject(answer);
      }

      private static Device ConvertDevice(OneWireContainer container)
      {
         var device = new Device
         {
            Family = (uint)(container.address[0] & 0xFF),
            Id = container.getAddressAsString().Substring(2, 12),
            FullId = container.getAddressAsString()
         };
         return device;
      }

      #endregion

      #region Read data request
      // Disable warning CS0649:
      // "Field '...' is never assigned to, and will always have its default value null"
#pragma warning disable 649
      public class ReadDataAnswer
      {
         public float Temperature;  // °C
         public float Humidity;     // %
         public bool DigitalIo;
         public UInt32 Counter;
         public Int32 Voltage;     // mV
      }
#pragma warning restore 649
      private string ProcessReadDataRequest(Request.ReadDataRequest readData)
      {
         if (!_devices.ContainsKey(readData.Id))
         {
            Logger.Logger.Log("ReadDataRequest on unknown device {0}", readData.Id);
            return JsonConvert.SerializeObject(new InvalidRequestAnswer { InvalidRequestReason = "UnknownDevice" });
         }

         OneWireAdapter.beginExclusive(true);
         var answer = DoReadDeviceData(readData);
         OneWireAdapter.endExclusive();

         return JsonConvert.SerializeObject(answer);
      }

      private object DoReadDeviceData(Request.ReadDataRequest readData)
      {
         var device = OneWireAdapter.getDeviceContainer(_devices[readData.Id].FullId);

         if (!device.isPresent())
         {
            Logger.Logger.Log("ReadDataRequest, selected device ({0}) is no longer present", readData.Id);
            return new InvalidRequestAnswer { InvalidRequestReason = "DeviceNotPresent" };
         }

         var switchDevice = device as SwitchContainer;
         if (switchDevice != null)
         {
            var rawData = switchDevice.readDevice();
            if (!switchDevice.hasLevelSensing())
            {
               Logger.Logger.Log("ReadDataRequest, selected device ({0}) has no level sensing", readData.Id);
               return new InvalidRequestAnswer { InvalidRequestReason = "TryToReadIoDeviceWithoutLevelSensing" };
            }
            if (readData.Unit >= switchDevice.getNumberChannels(rawData))
            {
               Logger.Logger.Log("ReadDataRequest, invalid unit number ({0}) for selected device ({1})", readData.Unit, readData.Id);
               return new InvalidRequestAnswer { InvalidRequestReason = "IoDeviceReadWithUnitNumberOutOfRange" };
            }
            return new ReadDataAnswer { DigitalIo = switchDevice.getLevel((int)readData.Unit, rawData) };
         }

         var temperatureDevice = device as TemperatureContainer;
         if (temperatureDevice != null)
         {
            var rawData = temperatureDevice.readDevice();
            temperatureDevice.doTemperatureConvert(rawData);
            rawData = temperatureDevice.readDevice();
            return new ReadDataAnswer { Temperature = (float)temperatureDevice.getTemperature(rawData) };
         }

         var humidityDevice = device as HumidityContainer;
         if (humidityDevice != null)
         {
            var rawData = humidityDevice.readDevice();
            return new ReadDataAnswer { Humidity = (float)humidityDevice.getHumidity(rawData) };
         }

         var potentiometerDevice = device as PotentiometerContainer;
         if (potentiometerDevice != null)
         {
            potentiometerDevice.readDevice();
            return new ReadDataAnswer { Voltage = potentiometerDevice.getWiperPosition() };
         }

         var adDevice = device as ADContainer;
         if (adDevice != null)
         {
            var rawData = adDevice.readDevice();
            if (readData.Unit >= adDevice.getNumberADChannels())
            {
               Logger.Logger.Log("ReadDataRequest, invalid unit number ({0}) for selected device ({1})", readData.Unit, readData.Id);
               return new InvalidRequestAnswer { InvalidRequestReason = "AdDeviceReadWithUnitNumberOutOfRange" };
            }
            return new ReadDataAnswer { Voltage = (int)(adDevice.getADVoltage((int)readData.Unit, rawData) * 1000) };
         }

         Logger.Logger.Log("ReadDataRequest, type {0} for selected device ({1}) is not supported", device.GetType(), readData.Id);
         return new InvalidRequestAnswer { InvalidRequestReason = "TryToReadUnsupportedDevice" };
      }
      #endregion

      #region Read chanels number request
      public class ReadNbChanelsAnswer
      {
         public int ChanelsNb;    // Channel number
      }

      private string ProcessReadChanelsNbRequest(Request.ReadNbChanelsRequest readData)
      {
         if (!_devices.ContainsKey(readData.Id))
         {
            Logger.Logger.Log("ReadChanelsNbRequest on unknown device {0}", readData.Id);
            return JsonConvert.SerializeObject(new InvalidRequestAnswer { InvalidRequestReason = "UnknownDevice" });
         }

         OneWireAdapter.beginExclusive(true);
         var answer = DoReadDeviceChanelsNb(readData);
         OneWireAdapter.endExclusive();

         return JsonConvert.SerializeObject(answer);
      }

      private object DoReadDeviceChanelsNb(Request.ReadNbChanelsRequest readData)
      {
         var device = OneWireAdapter.getDeviceContainer(_devices[readData.Id].FullId);

         if (!device.isPresent())
         {
            Logger.Logger.Log("ReadChanelsNbRequest, selected device ({0}) is no longer present", readData.Id);
            return new InvalidRequestAnswer { InvalidRequestReason = "DeviceNotPresent" };
         }

         var switchDevice = device as SwitchContainer;
         if (switchDevice != null)
         {
            var rawData = switchDevice.readDevice();
            return new ReadNbChanelsAnswer { ChanelsNb = switchDevice.getNumberChannels(rawData) };
         }

         var adDevice = device as ADContainer;
         if (adDevice != null)
         {
            adDevice.readDevice();
            return new ReadNbChanelsAnswer { ChanelsNb = adDevice.getNumberADChannels() };
         }

         Logger.Logger.Log("ReadDeviceChanelsNb, type {0} for selected device ({1}) is not supported", device.GetType(), readData.Id);
         return new InvalidRequestAnswer { InvalidRequestReason = "TryToReadChanelsNbUnsupportedDevice" };
      }

      #endregion

      #region Write data request
      internal class WriteAckAnswer { public bool Ack; }

      private string ProcessWriteDataRequest(Request.WriteDataRequest writeData)
      {
         if (!_devices.ContainsKey(writeData.Id))
         {
            Logger.Logger.Log("WriteDataRequest on unknown device {0}", writeData.Id);
            return "";
         }

         OneWireAdapter.beginExclusive(true);
         DoWriteDeviceData(writeData);
         OneWireAdapter.endExclusive();

         return JsonConvert.SerializeObject(new WriteAckAnswer { Ack = true });
      }

      private void DoWriteDeviceData(Request.WriteDataRequest writeData)
      {
         var device = OneWireAdapter.getDeviceContainer(_devices[writeData.Id].FullId);

         if (!device.isPresent())
         {
            Logger.Logger.Log("WriteDataRequest, selected device ({0}) is no longer present", writeData.Id);
            return;
         }

         var switchDevice = device as SwitchContainer;
         if (switchDevice != null)
         {
            var rawData = switchDevice.readDevice();
            if (writeData.Unit >= switchDevice.getNumberChannels(rawData))
            {
               Logger.Logger.Log("WriteDataRequest, invalid unit number ({0}) for selected device ({1})", writeData.Unit, writeData.Id);
               return;
            }
            switchDevice.setLatchState((int)writeData.Unit, writeData.Value, false, rawData);
            switchDevice.writeDevice(rawData);
         }
      }

      #endregion

      private void StartServer()
      {
         // Establish the local endpoint for the socket.
         // The DNS name of the computer
         // running the listener is "host.contoso.com".
         IPEndPoint localEndPoint = new IPEndPoint(IPAddress.IPv6Loopback, OneWireServicePort);

         // Create a TCP/IP socket.
         _serverSocket = new Socket(AddressFamily.InterNetworkV6, SocketType.Stream, ProtocolType.Tcp);

         // Bind the socket to the local endpoint and listen for incoming connections.
         try
         {
            _serverSocket.Bind(localEndPoint);
            _serverSocket.Listen(1);
         }
         catch (Exception e)
         {
            Logger.Logger.Log(e.ToString());
         }
      }

      public static string TheServiceName
      {
         get { return Program.Name + "Service"; }
      }
   }
}
