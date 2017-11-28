#eHouse BMS integration with Domoticz Rev.1

eHouse BMS is Building Management System and Home, Office, Hotel Automation -  from DIY to PRO (controllers, accessories, switchboards, software)

#eHouse TCP/IP & UDP Protocol: EN:www.isys.pl/download/ehouse-lan-protocol-en.pdf PL:www.isys.pl/download/ehouse-lan-protocol-pl.pdf
  - eHouse IP controllers base on UDP broadcast status which is available in Local Network (Ethernet+WiFi)
  - eHouse IP controllers base on TCP/IP sockets client/server for sending/receiving commands (eHouse Events)
  - each eHouse IP controller can send status via TCP/IP (sockets) to internet
  - eHouse.Pro Server gather all eHouse statuses and can send them to any client application via TCP/IP (sockets)

#Domoticz Configuration for eHouse System Integration:
- IP or DNS address of eHouse.PRO server (required for remote TCP/IP connection). Default PRO address: 192.168.0.200
- PORT (TCP/IP) - Server Port for all eHouse controlers & eHouse PRO server. Default TCP/IP port: 9876
- PORT (UDP) - Broadcast of controller statuses are distributed via local network on port: 6789 (non editable value)
- Password - password wchich will be used for dynamic code authorization to controllers 6 ascii chars. Default: "abBrac"
- AutoDiscovery - This mode automatically discover all eHouse Devices in the installation. 
- EnablePRO - Enable eHouse PRO direclty connected I/O buffers and BMS hardware (thermostats, temp sensors, RFID, etc)
- Enable Alam Inputs - future use. Add Alarm signals for each inputs (Early Warning, Monitoring, Silent Alarm, Horn, Warning Light). Increase Domoticz Input objects (~5..6 times)
- Option A - additional options (bitwise). Administrative purposes keep it to 0 - don't modify it unless you know exaclty what You are doing
- Option B - additional options (bitwise). Administrative purposes keep it to 0 - don't modify it unless you know exaclty what You are doing
 
 
#Integration/Connection of various eHouse IP Controllers:
- eHouse Ethernet: directly on LAN/WiFi Network. Do not require eHouse PRO server in LAN Network for continuous work of Domoticz - after complete auto-discovery of all controllers
- eHouse WiFi: directly on WiFi/LAN Network. Do not require eHouse PRO server in LAN Network for continuous work of Domoticz - after complete auto-discovery of all controllers
- eHouse PRO: directly on WiFi/LAN Network.

#Integration of various eHouse NON IP Controllers (VIA eHouse PRO Server & Communication gateways):
- eHouse RS-485: must work under Main Host Supervision: CommManager, LevelManager, eHouse.Exe or eHousePRO. Require eHouse PRO server (or other host) for continuous work for integration with Domoticz 
- eHouse Aura: Thermostats must work under Main Host Supervision: eHousePRO connected thru USB->AURA-485 gateway. Require eHouse PRO server for continuous work for integration with Domoticz 
- eHouse CAN/RF: currently not implemented (future) must work under Main Host Supervision: eHousePRO. Require eHouse PRO server (or other host) for continuous work for integration with Domoticz 
- eHouse RFID: currently not implemented (future) must work under Main Host Supervision: eHousePRO. Require eHouse PRO server (or other host) for continuous work for integration with Domoticz 
- eHouse BMS: other systems integrated with eHouse (future)

#eHouse Ethernet controllers are large  ( more than 50 intelligent points) microcontroller based units wchich can work standalone/autonomic after remote configuration via Windows application "eHouseWiFi.exe":
1) ERM (EthernetRoomManager) - Room Controller:  http://ehouse.biz/en/lan/erm3-diy.html , http://ehouse.biz/en/lan.html
	- up to 32 on/off outputs can work individually or paired for blinds/drives support with relay drives (for external relays)
	- up to 20 on/off programmable inputs (switches, sensors, etc) 
	- up to 15 ADC (0..3v3) measurement inputs for regulation (eg termostat)
	- up to 3   12-24VDC/PWM dimmers for direct LED light dimming
	- IR Receiver input for direct control of current controller (Room)
	- IR Transmiter output for direct control of external A/V systems (~23 IR standards implemented)
	- 128 items of calendar-scheduler for unatended and service operations
	- 24 light scenes/programs (on/off outputs + dimmers)
	- 12 ADC Measurement/Regulation Programs
	- 256 IR codes for reception (learning functionality)
	- 256 IR codes for transmission (learning functionality)
	- Domoticz Support: 
		- Single Outputs: on/off
		- Double Outputs (blinds):  open/close/set value
		- Dimmer (on/off/set value)
		- Thermostat Regulation Point (+/-/Set value)
		- Measurement input associated with Thermostat Preset
		- 10 Light Scenes/Programs (on/off outs + dimmers)
		- 10 ADC Measurement Programs
		- Inputs status Show
		- Room Plan Auto Creation
	
2) CM/LM (CommManager/LevelManager) - centralized controller with security system functionality:	http://ehouse.biz/en/lan/cm-diy.html
	- up to 77 on/off outputs can work individually (LM) or paired for blinds/drives support (CM)  with relay drives (for external relays)
	- up to 48 on/off programmable inputs (switches, sensors, etc) with security system functionality
	- up to 15 ADC (0..3v3) measurement inputs for regulation (eg termostat)
	- 128 items of calendar-scheduler for unatended and service operations
	- 16 light scenes/programs (on/off outputs + dimmers)
	- 12 ADC Measurement/Regulation Programs
	- 24 security zones
	- 24 roller programs (CM) / 16 scenes (LM)
	- Domoticz Support: 
		- Single Outputs: on/off
		- Double Outputs (blinds):  open/close/set value
		- Thermostat Regulation Point (+/-/Set value)
		- Measurement input associated with Thermostat Preset
		- 10 Blinds Scenes/Programs (on/off outs + dimmers)
		- 10 ADC Measurement Programs
		- Inputs status Show
		- Room Plan Auto Creation
		
3) EPM (Ethernet Pool Manager) - Swimming Pool Controller (dedicated firmware for optimal control of Swimming POOL): http://ehouse.biz/en/lan/automatyka-basenu.html
	- up to 32 on/off outputs can work individually or paired for blinds/drives support  with relay drives (for external relays)
	- up to 5 on/off programmable inputs (switches, sensors, etc) 
	- up to 15 ADC (0..3v3) measurement inputs for regulation (eg termostat)
	- up to 3   12-24VDC/PWM dimmers for direct LED light dimming
	- IR Receiver input for direct control of current controller (Room)
	- 128 items of calendar-scheduler for unatended and service operations
	- 24 light scenes/programs (on/off outputs + dimmers)
	- 12 ADC Measurement/Regulation Programs
	- 256 IR codes for reception (learning functionality)
	- Domoticz Support (In the future):
	
#eHouse WiFi 2.4GHz b/g/n controllers are medium size ( more than 10 intelligent points) microcontroller based units wchich can work standalone/autonomic after remote configuration via Windows application "eHouseWiFi.exe":
	http://ehouse.biz/en/wifi/ewifir.html
	- up to 4 on/off outputs can work individually or paired for blinds/drives support with relays 230V/5A
	- up to 4 on/off programmable inputs (switches, sensors, etc) 
	- up to 1 temperature measurement inputs for regulation (eg termostat)
	- up to 3   12-24VDC/PWM dimmers for direct LED light dimming
	- IR Receiver input for direct control of current controller or whole system
	- IR Transmiter output for direct control of external A/V systems (~23 IR standards implemented)
	- 256 IR codes for reception (learning functionality)
	- 256 IR codes for transmission (learning functionality)
	- optional 230V/5V build in power supply
	- Domoticz Support: 
		- Single Outputs: on/off
		- Double Outputs (blinds):  open/close/set value
		- Dimmer (on/off/set value)
		- Thermostat Regulation Point (+/-/Set value)
		- Measurement input associated with Thermostat Preset
		- Inputs status Show
		- Room Plan Auto Creation
	
#eHouse PRO/BMS:  http://ehouse.biz/en/bms.html
eHouse PRO are centralized Home Automation system based on Linux Microcomputer (Raspberry PI 1/2/3, Banana Pi/Pro, etc). With I/O Buffers and integrations (eHouse BMS, Thermostat, MySQL, Cloud, etc).
eHouse PRO server software without I/O buffers can also run on PC computers.

- up to 256 digital inputs via SPI
- up to 256 digital outputs via I2C
- up to 5 alarm outputs (Horn, Warning Light, Early Warning, Monitoring, Silent)
- security system functionality
- SMS/GSM Alarm notification and control
- WWW control
- TCP/IP Client/Server
- Modbus Client/Server
- Integrates (eHouse LAN, WiFi, RS-485, CAN/RF, Aura, RFID, Cloud, Satel Integra) into centralized/decentralized unlimited BMS system
- Domoticz Support: 
		- Single Outputs: on/off
		- Double Outputs (blinds):  open/close/set value
		- Inputs status Show
		- 10 Light Scenes/Programs (on/off outs)
		- 10 Blinds Scenes/Programs (open/close - double outputs )
		- 10 Security Zones (on/off inputs masks)
		- Room Plan Auto Creation
		
#eHouse RS-485 controllers are large  ( more than 49 intelligent points) microcontroller based units wchich can work standalone/autonomic after remote configuration via Windows application "eHouse.exe":
1) RM (RoomManager) - Room Controller: 	http://ehouse.biz/en/rs-485.html
	- up to 24 on/off outputs with relay drives (for external relays)
	- up to 12 on/off programmable inputs (switches, sensors, etc) 
	- up to 8 ADC (0..5) measurement inputs for regulation (eg termostat)
	- IR Receiver input for direct control of current controller (Room)
	- IR Transmiter output for direct control of external A/V systems (~23 IR standards implemented)
	- 248 items of calendar-scheduler for unatended and service operations
	- 24 light scenes/programs (on/off outputs + dimmers) + ADC Measurement/Regulation Programs
	- 248 IR codes for reception (learning functionality)
	- 248 IR codes for transmission (learning functionality)
	- Domoticz Support: 
		- Single Outputs: on/off
		- Measurement input show as temperature
		- 10 Light Scenes/Programs (on/off outs + dimmers)
		- 10 ADC Measurement Programs
		- Inputs status Show
		- Room Plan Auto Creation
2) HM (HeatManager) - Boiler Room, Central Heating, Ventilation Controller (dedicated firmware for optimal control of Heating): http://ehouse.biz/en/rs-485/hm-diy.html
	- up to 21 on/off outputs can work individually  with relay drives (for external relays)
	- up to 5 on/off programmable inputs (switches, sensors, etc) 
	- up to 16 ADC (0..5) measurement inputs for regulation (eg termostat)
	- up to 3   12-24VDC/PWM dimmers for direct LED light dimming
	- IR Receiver input for direct control of current controller (Room)
	- 128 items of calendar-scheduler for unatended and service operations
	- 24 light scenes/programs (on/off outputs + dimmers)
	- 12 ADC Measurement/Regulation Programs
	- 256 IR codes for reception (learning functionality)
3) CM/LM (CommManager/LevelManager) - centralized controller with security system functionality:	http://ehouse.biz/en/lan/cm-diy.html
	- up to 77 on/off outputs can work individually (LM) or paired for blinds/drives support (CM)  with relay drives (for external relays)
	- up to 48 on/off programmable inputs (switches, sensors, etc) with security system functionality
	- up to 15 ADC (0..3v3) measurement inputs for regulation (eg termostat)
	- 128 items of calendar-scheduler for unatended and service operations
	- 16 light scenes/programs (on/off outputs + dimmers)
	- 12 ADC Measurement/Regulation Programs
	- 24 security zones
	- 24 roller programs (CM) / 16 scenes (LM)
	- rs-485 interface for eHouse 1 supervising and integration

	
	
#Aura Wireless thermostat + temperature sensor
	- thermometer with LED Display
	- Thermostat Preset Local/Server
	- Domoticz Support: 
		- Thermostat Regulation Point (+/-/Set value)
		- Theromoeter associated with Thermostat Preset
		- Special Inputs status Show
		- Room Plan Auto Creation for all Aura controllers
		- 10 ADC Measurement Programs for all Aura controllers
		
#Step By step:
- Name all signals for each controller: unused devices should starting from '@' => signal will be ignored and won't add it to domoticz
- Configure all eHouse LAN, WiFi controllers with eHouseWiFi.exe application & press Save Settings (only via Ethernet)
- Configure all eHouse RS-485 controllers with eHouse.exe application & press Update settings (require eHouse RS-485 full duplex dongle)
- Configure eHouse PRO server (copy all eHouse LAN, WiFi, RS-485 controller settings if kept in other place)
- Configure all eHouse Aura Thermostat (require eHouse Aura Dongle connected to eHouse.PRO)
- Install/Compile Domoticz on eHouse PRO server Hardware (RPI2/3) or PC: recent version of repository :  https://github.com/ehousepro/domoticz/
- Configure Domoticz (Setup/Hardware/Add) - "eHouse UDP + TCP - LAN Interface"
	- Data Timeout: Disabled
	- Refresh ms: 1000
	- Default IP: 192.168.0.200 for eHouse PRO server used in local Ethernet/WiFi network. 
		In case of entering external public IP not in (192.168.x.x ) network eHouse Gateway for Domoticz will use TCP/IP connection to server instead of UDP.
	- Port: 9876 (TCP/IP) - to send events or for internet connection via TCP/IP
	- Password [6 ascii chars] Default:"abBrac". Password must be set the same for all eHouse controllers. Each controller must have "XOR Password" - dynamic authorisation set.
	- AutoDiscovery=1  - Automatically discover new eHouse controllers on restart of eHouse Service for Domoticz
	- EnablePRo=1 - Enable eHouse PRO centralized system
	- Alarm Inputs: 0 - Enable Alarm system (5 times more objects for each eHousePRO/CM inputs) - FUTURE
	- OptA=0,OptB=0 - bitwise administrative options (should be 0 for normal operation)	
- When all controllers will be automatically discovered AutoDiscovery flag can be disabled (however AutoDiscovery is initiated only on Restart of eHouse Hardware gateway for domoticz)	
- You can modify discovered names in Domoticz (dont change ID and other fields). Already existed signals (IDs) and names will not be overwriten.

#Domoticz Device ID construction hex coded: <eHouse Addr H> <eHouse Addr L> <Signal Type> <I/O Nr>
Prerequisities eHouse IP controllers must be on static IP - DHCP Settings  outside eHouse RANGE (LSB=50..99), MSB:0 strongly recomended:
eHouse PRO 			IP: 	192.168.0.200 (Default) 
eHouse LAN ERM  	IP:		192.168.0.201...248	(Required Range)  
eHouse LAN EPM  	IP:		192.168.0.249		(Required Range)  
eHouse LAN CM/LM  	IP:		192.168.0.254...250 (Required Range)  
eHouse WiFi			IP:		192.168.0.100...199 (Required Range)
Illegal values  u.x.y.z for eHouse LAN, PRO, WiFi controllers: y<>(1,2,55,85,120-140), 
Neccessary values u=192, x=168

y=1 allocated  for HeatManager (RS-485)
y=2 allocated for ExternalManager (RS-485)
y=55 allocated for RoomManagers (RS-485)
y=85 allocated for infrared barier (RS-485)
y=120..127 allocated for eHouse RF
y=128 allocated for eHouse CAN
y=129 allocated for eHouse AURA thermostat
y=130..140 future use

More Info EN:
	DIY: http://Smart.eHouse.PRO/
	SHOP: http://eHouse.BIZ/
	WWW/Docs: http://en.iSys.PL/

More Info PL:
	Info: http://eHouse.Info/
	DIY: http://iDom.eHouse.PRO/
	SHOP: http://eHouse.Net.PL/
	WWW/Docs: http://iSys.PL/

RU: 
	WWW: http://ru.iSys.PL/
UA: 
	WWW: http://ua.iSys.PL/

	
HW				OS			Compilation 			LAN UDP/TCP		Internet TCP/IP		100% Tested			eHouse Controllers
- PC x64(2,4,8)	Win 7		+VS2017 Community		(+)				(+)					(+) (*)				12*RM,HM, 12*AURA, PRO128/128, 12*ERM,CM, 22*WIFI (~1900 domoticz points/devices)
- PC x64		Win 8.1		+VS2017 Community		(-)				(-)					(-)					12*RM,HM, 12*AURA, PRO128/128, 12*ERM,CM, 22*WIFI (~1900 domoticz points/devices)
- PC x64		Win 10		+VS2017 Community		(-)				(-)					(-) (*)				12*RM,HM, 12*AURA, PRO128/128, 12*ERM,CM, 22*WIFI (~1900 domoticz points/devices)
- PC x64		Ubuntu 16		CMAKE				(+)				(+)					(+) (*)				12*RM,HM, 12*AURA, PRO128/128, 12*ERM,CM, 22*WIFI (~1900 domoticz points/devices)
- PC x64		Ubuntu 17		CMAKE				(+)				(+)					(+) (*)				12*RM,HM, 12*AURA, PRO128/128, 12*ERM,CM, 22*WIFI (~1900 domoticz points/devices)
- RPI1			Raspbian 		CMAKE				(-)				(-)					(-) 				(-) 1 ERM, PRO - stability test without WWW interface utilization (	~ 300 domoticz points/devices)
- RPI2			Raspbian		CMAKE				(+)				(-)					(-) (continuous)	12*RM,HM, 12*AURA, PRO128/128  (~1000 domoticz points/devices) + eHouse.PRO Server
- RPI3			Raspbian		CMAKE				(+)				(+)					(+) (*/continuous) 	12*RM,HM, 12*AURA, PRO128/128, 12*ERM,CM, 22*WIFI (~1900 domoticz points/devices)+ eHousePRO Server
(+) success
(-) not tested
(*) - sugested platforms


Stability and efficiency precautions
- eHouse Pro Server with eHouse PRO I/O processor utilization (~5-15%) RPI3
- Domoticz processor utilization (1-5%) 	RPI3

In case of serious efficiency problems:
- Domoticz software can be installed on another computer (RPI3 or more efficient 8 cores, PC, etc)
- Disable AutoDiscovery after system setting up system (no changes any longer)



