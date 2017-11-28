/*
 * File:   EventsCodes.h
 * Author: robert
 *
 * Created on 5 czerwiec 2014, 13:20
 */

#ifndef EVENTSCODES_H
#define	EVENTSCODES_H

#ifdef	__cplusplus
extern "C" {
#endif


	//Direct Events Commands
	//#define SETOUTPUT             1u              //set output for ehouse1 compatibility

#define EXECPROFILE             2u              //execute output program change
#define EXECPROFILE_EV          2u              //execute output program change

#define DIMMER_EVENT		4	
#define SETLIGHT_EV             4u              //set dimmer
#define SETLIGHT                4u              //set dimmer

#define RGB_DIMMER_EVENT	 8

#define SETADC_EV		12u             //set adc levels    
#define SETADC_EVENT            12             //ustawianie progów analogowych arg1=nr wej arg2=vlow arg3=vhigh    
#define SETADC                  12u             //set adc levels


#define INPUT_EVENT             0x20		//change input params

#define OUTPUT_EVENT                0x21		//output state
#define CMD_SETOUTPUTI2C_EV         0x21u		//change output state        
#define CMD_SETOUTPUTI2C            0x21u		//change output state

#define IRALIAS                 39u //x27             //send ir command
#define IRALIAS_EV		39u //x27            //send ir command


#define SATEL_INPUT_EVENT           0x30           //input state cfg for satel

#define SATEL_OUTPUT_EVENT          0x31           //output state for satel

#define SATEL_PROGRAM_EVENT         0x32          //set all outputs

#define SATEL_ZONE_CHANGE_EVENT     0x33          //Satel change zone

#define SATEL_ISOLATE_SENSOR_EVENT  0x34

#define SATEL_UNISOLATE_SENSOR_EVENT 0x35

#define SATEL_BYPASS_SENSOR_EVENT   0x36

#define SATEL_UNBYPASS_SENSOR_EVENT   0x37

#define SATEL_TOGGLEBYPASS_SENSOR_EVENT   0x38

#define SATEL_ROLLER_EVENT          0x39

#define SATEL_ROLLER_PROGRAM_EVENT  0x40



#define SATEL_ADC_PROGRAM_EVENT          0x41
#define SATEL_INPUT_MASK_CHANGE_EVENT    0x42

#define SATEL_TOGGLEISOLATE_SENSOR_EVENT 0x43

								//communication manager events
#define WRITEEEPROM		95u		//write 2 bytes of EEPROM SPI
#define WRITEEEPROM_EV          95u		//write 2 bytes of EEPROM SPI

#define INCDECPROFILE_EV	96u		//Increment / decrement Program
#define INCDECPROFILE		96u		//Increment / decrement Program

#define ADC_PROGRAM  		97u		//set adc program
#define ADC_PROGRAM_EV  	97u		//set adc program

#define SECU_PROGRAM_EV 	98u		//run rollers program toogether with zone
#define SECU_PROGRAM 		98u		//run rollers program toogether with zone

#define ZONE_CHANGE  		99u		//zone change	
#define ZONE_CHANGE_EV  	99u		//zone change	

#define ROLLERSSINGLE_EV 	100u		//single rollers execution    
#define ROLLER_EVENT		100u		//single rollers execution    
#define ROLLERSSINGLE 		100u		//single rollers execution

#define ROLLERSMULTI 		101u		//rollers program execution
#define ROLLERSMULTI_EV 	101u		//rollers program execution

#define ROLLERSMULTI2_EV 	102u    
#define ROLLERSMULTI2 		102u

#define ROLLERSMULTI3 		103u
#define ROLLERSMULTI3_EV 	103u

#define BATCH_ROM_EV    	104u		//run multiple events from rom bank    
#define BATCH_ROM    		104u		//run multiple events from rom bank

#define MODIFY_LIGHS		105u
#define MODIFY_LIGHS_EV         105u

#define SETSINGLELIGHT_EV	106u		  
#define SETSINGLELIGHT		106u		

#define SET_TIME		107u            //not supported here performed immediatelly
#define SET_TIME_EV             107u		//not supported here performed immediatelly

#define EVENT_INPUTS_CONFIG     108u

#define EMULATE_IN_EVENT        109u            //emulate set input event
#define EVENT_SATEL_ZONES       109u

#define EVENT_SATEL_OUTPUT      110u

#define EVENT_SATEL_PROGRAM     111u

#define EVENT_SATEL_SECUPROGRAMS 112u

#define EVENT_SATEL_ADC_PROGRAMS 113u



#define TRANSPARENT_UART_CAN        0xa5u		//UART <-> CAN gateway events: Set transparent mode for configuration

#define UART_SPEED_EVENT            0xa6u		//UART <-> CAN gateway events: Set Uart Speed temporarly (no writing to EEPROM)

#define CAN_SPEED_EVENT             0xa7u		//UART <-> CAN gateway events: Set CAN Speed Permanently

#define WRITE_EEPROM_EVENT          0xa8u		//UART <-> CAN gateway events: Write EEPROM memory (AddressH, AddresL, Data) in params

#define SET_TIME_EVENT              0xa9u 		//==169	//Send Time for other controlers
#define	MODADC			    245u                //modify ADC Settings
#define EVENT_SUBMIT_TCP_STATUS_EV  248u		//send #arg1-arg6 to log analyzer if connected
#define EVENT_SUBMIT_TCP_STATUS     248u            //send #arg1-arg6 to log analyzer if connected

#define EVENT_SEND_TCP              249u		//send #arg1-arg6 to opened TCP Query connection    
#define EVENT_SEND_TCP_EV           249u		//send #arg1-arg6 to opened TCP Query connection

#define EVENT_SEND_UDP_EV           250u		//broadcast #arg1-arg6 via udp 
#define EVENT_SEND_UDP              250u		//broadcast #arg1-arg6 via udp 

#define EVENT_SEND_TO_LOG           251u		//send #arg1-arg6 to log analyzer if connected

#define EVENT_SEND_TO_LOG_EV        251u		//send #arg1-arg6 to log analyzer if connected

#define SENDIRCOMMAND_EV            252u            //send infrared code #mode,#code6,#code5,#code4,#code3,#code2,#code1
#define SENDIRCOMMAND               252u            //send infrared code #mode,#code6,#code5,#code4,#code3,#code2,#code1

#define SCAN_IR                     253u		//scan IR remote controler
#define SCAN_IR_EV                  253u		//scan IR remote controler

#define RESETDEV_EV                 254u		//reset device
#define RESETDEV_EVENT              254u		//reset device,bootloader init
#define RESETDEV                    254u		//reset device








									//communication manager events




/*
//#define BUSY_EVENTS_CMDS		110u-140u for Visualization IDs
#define VISUAL_ADC_IN			110
#define VISUAL_TEMP_IN			111
#define VISUAL_LIGHT_IN			112
#define VISUAL_LM35_IN			113
#define VISUAL_LM335_IN			114
#define VISUAL_MCP9700_IN		115
#define VISUAL_MCP9701_IN		116
#define VISUAL_PERCENT_IN		117
#define VISUAL_INVERTED_PERCENT_IN	118
#define VISUAL_VOLTAGE_IN		119
#define VISUAL_INPUT_IN			121
#define VISUAL_ACTIVE_IN		122
#define VISUAL_WARNING_IN		123
#define VISUAL_MONITORING_IN            124
#define	VISUAL_ALARM_IN			125
#define VISUAL_DIMMER_OUT		126
 */


 //#define BUSY_EVENTS_CMDS			110u-140u for Visualization IDs
#define VISUAL_ADC_IN			110
#define VISUAL_TEMP_IN			111
#define VISUAL_LIGHT_IN			112
#define VISUAL_LM35_IN			113
#define VISUAL_LM335_IN			114
#define VISUAL_MCP9700_IN		115
#define VISUAL_MCP9701_IN		116
#define VISUAL_PERCENT_IN		117
#define VISUAL_INVERTED_PERCENT_IN	118
#define VISUAL_VOLTAGE_IN		119

#define VISUAL_INPUT_IN			121
#define VISUAL_ACTIVE_IN		122
#define VISUAL_WARNING_IN		123
#define VISUAL_MONITORING_IN            124
#define	VISUAL_ALARM_IN			125
#define VISUAL_DIMMER_OUT		126
#define VISUAL_HORN_IN                  127
#define VISUAL_EARLY_IN                 128
#define VISUAL_SMS_IN                   129
#define VISUAL_SILENT_IN                130



#define VISUAL_SATEL_INPUT_IN               131
#define VISUAL_SATEL_INPUT_ALARM_IN         132
#define	VISUAL_SATEL_INPUT_ALARM_MEM_IN     133  
#define VISUAL_SATEL_INPUT_NOVIOLATION_IN   134    
#define VISUAL_SATEL_INPUT_LONGVIOLATION_IN 135    

#define VISUAL_SATEL_TAMPER_IN              136
#define VISUAL_SATEL_TAMPER_ALARM_IN        137    
#define VISUAL_SATEL_TAMPER_ALARM_MEM_IN    138

#define VISUAL_SATEL_BYPAS_IN               139
#define VISUAL_SATEL_UNBYPAS_IN             140

#define VISUAL_SATEL_MASK_IN                141
#define VISUAL_SATEL_MASK_MEM_IN            142


#define VISUAL_SATEL_ISOLATE_IN             143

#define VISUAL_SATEL_OUT                    144

#ifdef	__cplusplus
}
#endif

#endif	/* EVENTSCODES_H */

