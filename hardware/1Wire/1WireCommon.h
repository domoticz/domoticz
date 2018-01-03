#pragma once

// 1-wire chip family definition (merge from http://owfs.sourceforge.net/family.html and http://owfs.org/index.php?page=family-code-list)
enum _e1WireFamilyType
{
   silicon_serial_number	= 0x01,
   multikey_1153bit_secure	= 0x02,
   econoram_time_chip	= 0x04,
   Addresable_Switch	= 0x05,
   _4k_memory_ibutton	= 0x06,
   _1k_memory_ibutton	= 0x08,
   _1k_add_only_memory	= 0x09,
   _16k_memory_ibutton	 = 0x0A,
   _16k_add_only_memory	 = 0x0B,
   _64k_memory_ibutton	 = 0x0C,
   _64k_add_only_memory	 = 0x0F,
   high_precision_digital_thermometer	= 0x10,
   dual_addressable_switch_plus_1k_memory	= 0x12,
   _256_eeprom	= 0x14,
   crypto_ibutton	= 0x16,
   SHA_iButton	= 0x18,
   _4k_Monetary	 = 0x1A,
   battery_id_monitor_chip	 = 0x1B,
   _4k_EEPROM_with_PIO	 = 0x1C,
   _4k_ram_with_counter	 = 0x1D,
   Battery_monitor	= 0x1E,
   microlan_coupler	 = 0x1F,
   quad_ad_converter	= 0x20,
   Thermachron	= 0x21,
   Econo_Digital_Thermometer	= 0x22,
   _4k_eeprom	= 0x23,
   time_chip	= 0x24,
   smart_battery_monitor	= 0x26,
   time_chip_with_interrupt	= 0x27,
   programmable_resolution_digital_thermometer	= 0x28,
   _8_channel_addressable_switch	= 0x29,
   digital_potentiometer	 = 0x2C,
   _1k_eeprom	 = 0x2D,
   battery_monitor_and_charge_controller	 = 0x2E,
   high_precision_li_battery_monitor	= 0x30,
   efficient_addressable_single_cell_rechargable_lithium_protection_ic	= 0x31,
   Battery = 0x32,
   _1k_protected_eeprom_with_SHA_1	= 0x33,
   SHA_1_Battery = 0x34,
   Battery_Fuel_Gauge = 0x35,
   high_precision_coulomb_counter	= 0x36,
   Password_protected_32k_eeprom	= 0x37,
   dual_channel_addressable_switch	 = 0x3A,
   Temperature_memory = 0x3B,
   stand_alone_fuel_gauge = 0x3D,
   Temperature_Logger_8k_mem	= 0x41,
   Temperature_IO	= 0x42,
   Memory = 0x43,
   SHA_1_Authenticator = 0x44,
   multichemistry_battery_fuel_gauge	= 0x51,
   Environmental_Monitors = 0x7E,
   Serial_ID_Button	= 0x81,
   Authorization = 0x82,
   dual_port_plus_time	= 0x84,
   _48_bit_node_address_chip	= 0x89,
   _16k_add_only_uniqueware	 = 0x8B,
   _64k_add_only_uniqueware	 = 0x8F,
   Rotation_Sensor = 0xA0,
   Vibratio	= 0xA1,
   AC_Voltage	= 0xA2,
   IR_Temperature	= 0xA6,
   _21_channels_thermocouple_Converter	= 0xB1,
   DC_Current_or_Voltage	= 0xB2,
   _168_channels_thermocouple_Converter	= 0xB3,
   Ultra_Violet_Index	= 0xEE,
   Moisture_meter_4_Channel_Hub	= 0xEF,
   Programmable_Miroprocessor	= 0xFC,
   LCD_Swart	 = 0xFF,

   _e1WireFamilyTypeUnknown = 0
};

struct _t1WireDevice
{
   _e1WireFamilyType family;
   std::string devid;
   std::string filename;

   bool operator<(_t1WireDevice other) const
   {
	   return devid > other.devid;
   }
};

_e1WireFamilyType ToFamily(const std::string& str);


#define DEVICE_ID_SIZE  6
void DeviceIdToByteArray(const std::string &deviceId,/*out*/unsigned char* byteArray);
std::string ByteArrayToDeviceId(const unsigned char* byteArray);

unsigned char Crc16(const unsigned char* byteArray,size_t arraySize);
