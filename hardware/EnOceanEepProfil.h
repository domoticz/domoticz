
// function number :0x01 : Electronic switches and dimmers with Energy Measurement and Local Control 
// function type :0x00 : Type 0x00 
//{ 0xD2, 0x01, 0x00, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x00                                                                       " },

// TITLE:CMD 0x1 - Actuator Set Output
// DESC :This message is sent to an actuator. It controls switching / dimming of one or all channels of an actuator.
enocean::T_DATAFIELD D20100_CMD1 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x01 = ID 01 
{  8 , 3 ,     0 ,     0 ,     0 ,     0 , "DV"       , "Dim value"},//Value: 0x00 = Switch to new output value 
                                                                     //Value: 0x01 = Dim to new output value - dim timer 1 
                                                                     //Value: 0x02 = Dim to new output value - dim timer 2 
                                                                     //Value: 0x03 = Dim to new output value - dim timer 3 
                                                                     //Value: 0x04 = Stop dimming 

{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 17 , 7 ,     0 ,     0 ,     0 ,     0 , "OV"       , "Output value"},//Value: 0x00 = Output value 0% or OFF 

{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE1 = { D20100_CMD1,"CMD 0x1 - Actuator Set Output","This message is sent to an actuator. It controls switching / dimming of one or all channels of an actuator." } ;// Index of field
#define D20100_CMD1_CMD        0
#define D20100_CMD1_DV         1
#define D20100_CMD1_I_O        2
#define D20100_CMD1_OV         3
#define D20100_CMD1_NB_DATA    4
#define D20100_CMD1_DATA_SIZE  3

// TITLE:CMD 0x2 - Actuator Set Local
// DESC :This message is sent to an actuator. It configures one or all channels of an actuator.
enocean::T_DATAFIELD D20100_CMD2 [] = {
{  0 , 1 ,     0 ,     0 ,     0 ,     0 , "d_e"      , "Taught-in devices"},//Value: 0b0 = Disable taught-in devices (with different EEP) 
                                                                             //Value: 0b1 = Enable taught-in devices (with different EEP) 
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x02 = ID 02 
{  8 , 1 ,     0 ,     0 ,     0 ,     0 , "OC"       , "Over current shut down"},//Value: 0b0 = Over current shut down: static off 
                                                                                  //Value: 0b1 = Over current shut down: automatic restart 
{  9 , 1 ,     0 ,     0 ,     0 ,     0 , "RO"       , "reset over current shut down"},//Value: 0b0 = Reset over current shut down: not active 
                                                                                        //Value: 0b1 = Reset over current shut down: trigger signal 
{ 10 , 1 ,     0 ,     0 ,     0 ,     0 , "LC"       , "Local control"},//Value: 0b0 = Disable local control 
                                                                         //Value: 0b1 = Enable local control 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 20 , 4 ,     0 ,     0 ,     0 ,     0 , "DT3"      , "Dim timer 3"},//Value: 0x00 = Not used 

{ 28 , 4 ,     0 ,     0 ,     0 ,     0 , "DT1"      , "Dim timer 1"},//Value: 0x00 = Not used 

{ 16 , 4 ,     0 ,     0 ,     0 ,     0 , "DT2"      , "Dim timer 2"},//Value: 0x00 = Not used 

{ 24 , 1 ,     0 ,     0 ,     0 ,     0 , "d_n"      , "User interface indication"},//Value: 0b0 = User interface indication: day operation 
                                                                                     //Value: 0b1 = User interface indication: night operation 
{ 25 , 1 ,     0 ,     0 ,     0 ,     0 , "PF"       , "Power Failure"},//Value: 0b0 = Disable Power Failure Detection 
                                                                         //Value: 0b1 = Enable Power Failure Detection 
{ 26 , 2 ,     0 ,     0 ,     0 ,     0 , "DS"       , "Default state"},//Value: 0b00 = Default state: 0% or OFF 
                                                                         //Value: 0b01 = Default state: 100% or ON 
                                                                         //Value: 0b10 = Default state: remember previous state 
                                                                         //Value: 0b11 = Not used 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE2 = { D20100_CMD2,"CMD 0x2 - Actuator Set Local","This message is sent to an actuator. It configures one or all channels of an actuator." } ;// Index of field
#define D20100_CMD2_d_e        0
#define D20100_CMD2_CMD        1
#define D20100_CMD2_OC         2
#define D20100_CMD2_RO         3
#define D20100_CMD2_LC         4
#define D20100_CMD2_I_O        5
#define D20100_CMD2_DT3        6
#define D20100_CMD2_DT1        7
#define D20100_CMD2_DT2        8
#define D20100_CMD2_d_n        9
#define D20100_CMD2_PF         10
#define D20100_CMD2_DS         11
#define D20100_CMD2_NB_DATA    12
#define D20100_CMD2_DATA_SIZE  4

// TITLE:CMD 0x3 - Actuator Status Query
// DESC :This message is sent to an actuator. It requests the status of one or all channels of an actuator.
enocean::T_DATAFIELD D20100_CMD3 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x03 = ID 03 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE3 = { D20100_CMD3,"CMD 0x3 - Actuator Status Query","This message is sent to an actuator. It requests the status of one or all channels of an actuator." } ;// Index of field
#define D20100_CMD3_CMD        0
#define D20100_CMD3_I_O        1
#define D20100_CMD3_NB_DATA    2
#define D20100_CMD3_DATA_SIZE  2

// TITLE:CMD 0x4 - Actuator Status Response
// DESC :This message is sent by an actuator if one of the following events occurs:
enocean::T_DATAFIELD D20100_CMD4 [] = {
{  0 , 1 ,     0 ,     0 ,     0 ,     0 , "PF"       , "Power Failure"},//Value: 0b0 = Power Failure Detection disabled/not supported 
                                                                         //Value: 0b1 = Power Failure Detection enabled 
{  1 , 1 ,     0 ,     0 ,     0 ,     0 , "PFD"      , "Power Failure Detection"},//Value: 0b0 = Power Failure not detected/not supported/disabled 
                                                                                   //Value: 0b1 = Power Failure Detected 
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x04 = ID 04 
{  8 , 1 ,     0 ,     0 ,     0 ,     0 , "OC"       , "Over current switch off"},//Value: 0b0 = Over current switch off: ready / not supported 
                                                                                   //Value: 0b1 = Over current switch off: executed 
{  9 , 2 ,     0 ,     0 ,     0 ,     0 , "EL"       , "Error level"},//Value: 0b00 = Error level 0: hardware OK 
                                                                       //Value: 0b01 = Error level 1: hardware warning 
                                                                       //Value: 0b10 = Error level 2: hardware failure 
                                                                       //Value: 0b11 = Error level not supported 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 16 , 1 ,     0 ,     0 ,     0 ,     0 , "LC"       , "Local control"},//Value: 0b0 = Local control disabled / not supported 
                                                                         //Value: 0b1 = Local control enabled 
{ 17 , 7 ,     0 ,     0 ,     0 ,     0 , "OV"       , "Output value"},//Value: 0x00 = Output value 0% or OFF 

{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE4 = { D20100_CMD4,"CMD 0x4 - Actuator Status Response","This message is sent by an actuator if one of the following events occurs:" } ;// Index of field
#define D20100_CMD4_PF         0
#define D20100_CMD4_PFD        1
#define D20100_CMD4_CMD        2
#define D20100_CMD4_OC         3
#define D20100_CMD4_EL         4
#define D20100_CMD4_I_O        5
#define D20100_CMD4_LC         6
#define D20100_CMD4_OV         7
#define D20100_CMD4_NB_DATA    8
#define D20100_CMD4_DATA_SIZE  3

// TITLE:CMD 0x5 - Actuator Set Measurement
// DESC :The command defines values at offset 32 and at offset 40 which are the limits for the transmission periodicity of messages. MIT must not be set to 0, MAT >= MIT.
enocean::T_DATAFIELD D20100_CMD5 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x05 = ID 05 
{  8 , 1 ,     0 ,     0 ,     0 ,     0 , "RM"       , "Report measurement"},//Value: 0b0 = Report measurement: query only 
                                                                              //Value: 0b1 = Report measurement: query / auto reporting 
{  9 , 1 ,     0 ,     0 ,     0 ,     0 , "RE"       , "Reset measurement"},//Value: 0b0 = Reset measurement: not active 
                                                                             //Value: 0b1 = Reset measurement: trigger signal 
{ 10 , 1 ,     0 ,     0 ,     0 ,     0 , "e_p"      , "Measurement mode"},//Value: 0b0 = Energy measurement 
                                                                            //Value: 0b1 = Power measurement 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 16 , 4 ,     0 ,  4095 ,     0 ,  4095 , "MD_LSB"   , "Measurement delta to be reported (LSB)"},
{ 24 , 8 ,     0 ,  4095 ,     0 ,  4095 , "MD_MSB"   , "Measurement delta to be reported (MSB)"},
{ 21 , 3 ,     0 ,     0 ,     0 ,     0 , "UN"       , "Unit"},//Value: 0x00 = Energy [Ws] 
                                                                //Value: 0x01 = Energy [Wh] 
                                                                //Value: 0x02 = Energy [KWh] 
                                                                //Value: 0x03 = Power [W] 
                                                                //Value: 0x04 = Power [KW] 

{ 32 , 8 ,     0 ,     0 ,     0 ,     0 , "MAT"      , "Maximum time between two subsequent actuator messages"},
{ 40 , 8 ,     0 ,     0 ,     0 ,     0 , "MIT"      , "Minimum time between two subsequent actuator messages"},
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE5 = { D20100_CMD5,"CMD 0x5 - Actuator Set Measurement","The command defines values at offset 32 and at offset 40 which are the limits for the transmission periodicity of messages. MIT must not be set to 0, MAT >= MIT." } ;// Index of field
#define D20100_CMD5_CMD        0
#define D20100_CMD5_RM         1
#define D20100_CMD5_RE         2
#define D20100_CMD5_e_p        3
#define D20100_CMD5_I_O        4
#define D20100_CMD5_MD_LSB     5
#define D20100_CMD5_MD_MSB     6
#define D20100_CMD5_UN         7
#define D20100_CMD5_MAT        8
#define D20100_CMD5_MIT        9
#define D20100_CMD5_NB_DATA    10
#define D20100_CMD5_DATA_SIZE  6

// TITLE:CMD 0x6 - Actuator Measurement Query
// DESC :This message is sent to an actuator. The actuator replies with an Actuator Measurement Response message.
enocean::T_DATAFIELD D20100_CMD6 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x06 = ID 06 
{ 10 , 1 ,     0 ,     0 ,     0 ,     0 , "qu"       , "Query"},//Value: 0b0 = Query energy 
                                                                 //Value: 0b1 = Query power 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE6 = { D20100_CMD6,"CMD 0x6 - Actuator Measurement Query","This message is sent to an actuator. The actuator replies with an Actuator Measurement Response message." } ;// Index of field
#define D20100_CMD6_CMD        0
#define D20100_CMD6_qu         1
#define D20100_CMD6_I_O        2
#define D20100_CMD6_NB_DATA    3
#define D20100_CMD6_DATA_SIZE  2

// TITLE:CMD 0x7 - Actuator Measurement Response
// DESC :This message is sent by an actuator if one of the following events occurs:
enocean::T_DATAFIELD D20100_CMD7 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x07 = ID 07 
{  8 , 3 ,     0 ,     0 ,     0 ,     0 , "UN"       , "Unit"},//Value: 0x00 = Energy [Ws] 
                                                                //Value: 0x01 = Energy [Wh] 
                                                                //Value: 0x02 = Energy [KWh] 
                                                                //Value: 0x03 = Power [W] 
                                                                //Value: 0x04 = Power [KW] 

{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 16 ,32 ,     0 , 4294967295 ,     0 ,     0 , "MV"       , "Measurement value (4 bytes)"},
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE7 = { D20100_CMD7,"CMD 0x7 - Actuator Measurement Response","This message is sent by an actuator if one of the following events occurs:" } ;// Index of field
#define D20100_CMD7_CMD        0
#define D20100_CMD7_UN         1
#define D20100_CMD7_I_O        2
#define D20100_CMD7_MV         3
#define D20100_CMD7_NB_DATA    4
#define D20100_CMD7_DATA_SIZE  6

// TITLE:CMD 0x8 - Actuator Set Pilot Wire Mode
// DESC :
enocean::T_DATAFIELD D20100_CMD8 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x08 = ID 08 
{ 13 , 3 ,     0 ,     0 ,     0 ,     0 , "PM"       , "Pilotwire mode"},//Value: 0x00 = Off 
                                                                          //Value: 0x01 = Comfort 
                                                                          //Value: 0x02 = Eco 
                                                                          //Value: 0x03 = Anti-freeze 
                                                                          //Value: 0x04 = Comfort-1 
                                                                          //Value: 0x05 = Comfort-2 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE8 = { D20100_CMD8,"CMD 0x8 - Actuator Set Pilot Wire Mode","" } ;// Index of field
#define D20100_CMD8_CMD        0
#define D20100_CMD8_PM         1
#define D20100_CMD8_NB_DATA    2
#define D20100_CMD8_DATA_SIZE  2

// TITLE:CMD 0x9 - Actuator Pilot Wire Mode Query
// DESC :
enocean::T_DATAFIELD D20100_CMD9 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x09 = ID 09 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE9 = { D20100_CMD9,"CMD 0x9 - Actuator Pilot Wire Mode Query","" } ;// Index of field
#define D20100_CMD9_CMD        0
#define D20100_CMD9_NB_DATA    1
#define D20100_CMD9_DATA_SIZE  1

// TITLE:CMD 0xA - Actuator Pilot Wire Mode Response
// DESC :
enocean::T_DATAFIELD D20100_CMD10 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x0A = ID 0A 
{ 13 , 3 ,     0 ,     0 ,     0 ,     0 , "PM"       , "Pilotwire mode"},//Value: 0x00 = Off 
                                                                          //Value: 0x01 = Comfort 
                                                                          //Value: 0x02 = Eco 
                                                                          //Value: 0x03 = Anti-freeze 
                                                                          //Value: 0x04 = Comfort-1 
                                                                          //Value: 0x05 = Comfort-2 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE10 = { D20100_CMD10,"CMD 0xA - Actuator Pilot Wire Mode Response","" } ;// Index of field
#define D20100_CMD10_CMD        0
#define D20100_CMD10_PM         1
#define D20100_CMD10_NB_DATA    2
#define D20100_CMD10_DATA_SIZE  2

// TITLE:CMD 0xB - Actuator Set External Interface Settings
// DESC :
enocean::T_DATAFIELD D20100_CMD11 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x0B = ID 0B 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 16 ,16 ,     0 ,     0 ,     0 ,     0 , "AOT"      , "Auto OFF Timer"},//Value: 0x0000 = Timer deactivated 

{ 32 ,16 ,     0 ,     0 ,     0 ,     0 , "DOT"      , "Delay OFF Timer"},//Value: 0x0000 = Timer deactivated 

{ 48 , 2 ,     0 ,     0 ,     0 ,     0 , "EBM"      , "External Switch/Push Button"},//Value: 0b00 = Not applicable 
                                                                                       //Value: 0b01 = External Switch 
                                                                                       //Value: 0b10 = External Push Button 
                                                                                       //Value: 0b11 = Auto detect 
{ 50 , 1 ,     0 ,     0 ,     0 ,     0 , "SWT"      , "2-state switch"},//Value: 0b00 = Change of key state sets ON or OFF 
                                                                          //Value: 0b01 = Specific ON/OFF positions. 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE11 = { D20100_CMD11,"CMD 0xB - Actuator Set External Interface Settings","" } ;// Index of field
#define D20100_CMD11_CMD        0
#define D20100_CMD11_I_O        1
#define D20100_CMD11_AOT        2
#define D20100_CMD11_DOT        3
#define D20100_CMD11_EBM        4
#define D20100_CMD11_SWT        5
#define D20100_CMD11_NB_DATA    6
#define D20100_CMD11_DATA_SIZE  7

// TITLE:CMD 0xC - Actuator External Interface Settings Query
// DESC :
enocean::T_DATAFIELD D20100_CMD12 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x0C = ID 0C 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE12 = { D20100_CMD12,"CMD 0xC - Actuator External Interface Settings Query","" } ;// Index of field
#define D20100_CMD12_CMD        0
#define D20100_CMD12_I_O        1
#define D20100_CMD12_NB_DATA    2
#define D20100_CMD12_DATA_SIZE  2

// TITLE:CMD 0xD - Actuator External Interface Settings Response
// DESC :
enocean::T_DATAFIELD D20100_CMD13 [] = {
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 0x0D = ID 0D 
{ 11 , 5 ,     0 ,     0 ,     0 ,     0 , "I_O"      , "I/O channel"},
{ 16 ,16 ,     0 ,     0 ,     0 ,     0 , "AOT"      , "Auto OFF Timer"},//Value: 0x0000 = Timer deactivated 

{ 32 ,16 ,     0 ,     0 ,     0 ,     0 , "DOT"      , "Delay OFF Timer"},//Value: 0x0000 = Timer deactivated 

{ 48 , 2 ,     0 ,     0 ,     0 ,     0 , "EBM"      , "External Switch/Push Button"},//Value: 0b00 = Not applicable 
                                                                                       //Value: 0b01 = External Switch 
                                                                                       //Value: 0b10 = External Push Button 
                                                                                       //Value: 0b11 = Auto detect 
{ 50 , 1 ,     0 ,     0 ,     0 ,     0 , "SWT"      , "2-state switch"},//Value: 0b00 = Change of key state sets ON or OFF 
                                                                          //Value: 0b01 = Specific ON/OFF positions. 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D20100_CASE13 = { D20100_CMD13,"CMD 0xD - Actuator External Interface Settings Response","" } ;// Index of field
#define D20100_CMD13_CMD        0
#define D20100_CMD13_I_O        1
#define D20100_CMD13_AOT        2
#define D20100_CMD13_DOT        3
#define D20100_CMD13_EBM        4
#define D20100_CMD13_SWT        5
#define D20100_CMD13_NB_DATA    6
#define D20100_CMD13_DATA_SIZE  7

enocean::T_EEP_CASE_* D20100_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0 
};

// function type :0x01 : Type 0x01 (description: see table) 
//{ 0xD2, 0x01, 0x01, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x01 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20101_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0 
};

// function type :0x02 : Type 0x02 (description: see table) 
//{ 0xD2, 0x01, 0x02, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x02 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20102_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x03 : Type 0x03 (description: see table) 
//{ 0xD2, 0x01, 0x03, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x03 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20103_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x04 : Type 0x04 (description: see table) 
//{ 0xD2, 0x01, 0x04, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x04 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20104_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x05 : Type 0x05 (description: see table) 
//{ 0xD2, 0x01, 0x05, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x05 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20105_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x06 : Type 0x06 (description: see table) 
//{ 0xD2, 0x01, 0x06, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x06 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20106_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x07 : Type 0x07 (description: see table) 
//{ 0xD2, 0x01, 0x07, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x07 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20107_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x08 : Type 0x08 (description: see table) 
//{ 0xD2, 0x01, 0x08, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x08 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20108_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x09 : Type 0x09 (description: see table) 
//{ 0xD2, 0x01, 0x09, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x09 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20109_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x0A : Type 0x0A (description: see table) 
//{ 0xD2, 0x01, 0x0A, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x0A (description: see table)                                              " },

enocean::T_EEP_CASE_* D2010A_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x0B : Type 0x0B (description: see table) 
//{ 0xD2, 0x01, 0x0B, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x0B (description: see table)                                              " },

enocean::T_EEP_CASE_* D2010B_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x0C : Type 0x0C 
//{ 0xD2, 0x01, 0x0C, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x0C                                                                       " },

enocean::T_EEP_CASE_* D2010C_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x0D : Type 0x0D 
//{ 0xD2, 0x01, 0x0D, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x0D                                                                       " },

enocean::T_EEP_CASE_* D2010D_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x0E : Type 0x0E 
//{ 0xD2, 0x01, 0x0E, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x0E                                                                       " },

enocean::T_EEP_CASE_* D2010E_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x0F : Type 0x0F 
//{ 0xD2, 0x01, 0x0F, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x0F                                                                       " },

enocean::T_EEP_CASE_* D2010F_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x10 : Type 0x10 (description: see table) 
//{ 0xD2, 0x01, 0x10, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x10 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20110_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x11 : Type 0x11 (description: see table) 
//{ 0xD2, 0x01, 0x11, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x11 (description: see table)                                              " },

enocean::T_EEP_CASE_* D20111_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};

// function type :0x12 : Type 0x12 
//{ 0xD2, 0x01, 0x12, "Electronic switches and dimmers with Energy Measurement and Local Control       " , "Type 0x12                                                                       " },

enocean::T_EEP_CASE_* D20112_CASES [] = {
&D20100_CASE1 ,
&D20100_CASE2 ,
&D20100_CASE3 ,
&D20100_CASE4 ,
&D20100_CASE5 ,
&D20100_CASE6 ,
&D20100_CASE7 ,
&D20100_CASE8 ,
&D20100_CASE9 ,
&D20100_CASE10 ,
&D20100_CASE11 ,
&D20100_CASE12 ,
&D20100_CASE13 ,
0
};


// function number :0x05 : Blinds Control for Position and Angle 
// function type :0x00 : Type 0x00 
//{ 0xD2, 0x05, 0x00, "Blinds Control for Position and Angle                                           " , "Type 0x00                                                                       " },

// TITLE:CMD 1 - Go to Position and Angle
// DESC :Once the actuator is configured either by the  Set Parameters  command or through manual configuration (using local buttons) the position of the blinds can be controlled with this command.
enocean::T_DATAFIELD D2050X_CMD1 [] = {
{  1 , 7 ,     0 ,     0 ,     0 ,     0 , "POS"      , "Position"},
{  9 , 7 ,     0 ,     0 ,     0 ,     0 , "ANG"      , "Angle"},
{ 17 , 3 ,     0 ,     0 ,     0 ,     0 , "REPO"     , "Repositioning"},//Value: 0 = Go directly to POS/ANG 
                                                                         //Value: 1 = Go up (0%), then to POS/ANG 
                                                                         //Value: 2 = Go down (100%), then to POS/ANG 
                                                                         //Value: 3 ... 7 = Reserved 
{ 21 , 3 ,     0 ,     0 ,     0 ,     0 , "LOCK"     , "Locking modes"},//Value: 0 = Do not change 
                                                                         //Value: 1 = Set blockage mode 
                                                                         //Value: 2 = Set alarm mode 
                                                                         //Value: 3 ... 6 = Reserved 
                                                                         //Value: 7 = Deblockage 
{ 24 , 4 ,     0 ,     0 ,     0 ,     0 , "CHN"      , "Channel"},//Value: 0 = Channel 1 
{ 28 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 1 = Goto command 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D2050X_CASE1 = { D2050X_CMD1,"CMD 1 - Go to Position and Angle","Once the actuator is configured either by the  Set Parameters  command or through manual configuration (using local buttons) the position of the blinds can be controlled with this command." } ;// Index of field
#define D2050X_CMD1_POS        0
#define D2050X_CMD1_ANG        1
#define D2050X_CMD1_REPO       2
#define D2050X_CMD1_LOCK       3
#define D2050X_CMD1_CHN        4
#define D2050X_CMD1_CMD        5
#define D2050X_CMD1_NB_DATA    6
#define D2050X_CMD1_DATA_SIZE  4

// TITLE:CMD 2 - Stop
// DESC :This command immediately stops a running blind motor. It has no effect when the actuator is in  blockage  or  alarm  mode, i.e. it will not stop an eventual  go up  or  go down  alarm action.
enocean::T_DATAFIELD D2050X_CMD2 [] = {
{  0 , 4 ,     0 ,     0 ,     0 ,     0 , "CHN"      , "Channel"},//Value: 0 = Channel 1 
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 2 = Stop command 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D2050X_CASE2 = { D2050X_CMD2,"CMD 2 - Stop","This command immediately stops a running blind motor. It has no effect when the actuator is in  blockage  or  alarm  mode, i.e. it will not stop an eventual  go up  or  go down  alarm action." } ;// Index of field
#define D2050X_CMD2_CHN        0
#define D2050X_CMD2_CMD        1
#define D2050X_CMD2_NB_DATA    2
#define D2050X_CMD2_DATA_SIZE  1

// TITLE:CMD 3 - Query Position and Angle
// DESC :This command requests the actuator to return a  reply  command.
enocean::T_DATAFIELD D2050X_CMD3 [] = {
{  0 , 4 ,     0 ,     0 ,     0 ,     0 , "CHN"      , "Channel"},//Value: 0 = Channel 1 
{  4 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 3 = Query command 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D2050X_CASE3 = { D2050X_CMD3,"CMD 3 - Query Position and Angle","This command requests the actuator to return a  reply  command." } ;// Index of field
#define D2050X_CMD3_CHN        0
#define D2050X_CMD3_CMD        1
#define D2050X_CMD3_NB_DATA    2
#define D2050X_CMD3_DATA_SIZE  1

// TITLE:CMD 4 - Reply Position and Angle
// DESC :Either upon request ( Query  command) or after an internal trigger (see EEP Properties) the actuator sends this command to inform about its current state.
enocean::T_DATAFIELD D2050X_CMD4 [] = {
{  1 , 7 ,     0 ,     0 ,     0 ,     0 , "POS"      , "Position"},
{  9 , 7 ,     0 ,     0 ,     0 ,     0 , "ANG"      , "Angle"},
{ 21 , 3 ,     0 ,     0 ,     0 ,     0 , "LOCK"     , "Locking modes"},//Value: 0 = Normal (no lock) 
                                                                         //Value: 1 = Blockage mode 
                                                                         //Value: 2 = Alarm mode 
                                                                         //Value: 3 ... 7 = Reserved 
{ 24 , 4 ,     0 ,     0 ,     0 ,     0 , "CHN"      , "Channel"},//Value: 0 = Channel 1 
{ 28 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 4 = Reply command 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D2050X_CASE4 = { D2050X_CMD4,"CMD 4 - Reply Position and Angle","Either upon request ( Query  command) or after an internal trigger (see EEP Properties) the actuator sends this command to inform about its current state." } ;// Index of field
#define D2050X_CMD4_POS        0
#define D2050X_CMD4_ANG        1
#define D2050X_CMD4_LOCK       2
#define D2050X_CMD4_CHN        3
#define D2050X_CMD4_CMD        4
#define D2050X_CMD4_NB_DATA    5
#define D2050X_CMD4_DATA_SIZE  4

// TITLE:CMD 5 - Set parameters
// DESC :This command sets one or multiple configuration parameters of the actuator. When a parameter value is set to  -> no change  this parameter will not be modified. The VERT and ROT parameters describe the duration needed by the motor for a full run of the blind, or for a complete turn of the slats, respectively. They have to be measured on site and assigned to the actuator.
enocean::T_DATAFIELD D2050X_CMD5 [] = {
{  1 ,15 ,     0 ,     0 ,     0 ,     0 , "VERT"     , "Set vertical"},
{ 16 , 8 ,     0 ,     0 ,     0 ,     0 , "ROT"      , "Set rotation"},
{ 29 , 3 ,     0 ,     0 ,     0 ,     0 , "AA"       , "Set alarm action"},//Value: 0 = No action 
                                                                            //Value: 1 = Immediate stop 
                                                                            //Value: 2 = Go up (0%) 
                                                                            //Value: 3 = Go down (100%) 
                                                                            //Value: 4 ... 6 = Reserved 
                                                                            //Value: 7 = -> No change 
{ 32 , 4 ,     0 ,     0 ,     0 ,     0 , "CHN"      , "Channel"},//Value: 0 = Channel 1 
{ 36 , 4 ,     0 ,     0 ,     0 ,     0 , "CMD"      , "Command ID"},//Value: 5 = Set parameters command 
{  0 , 0 , 0          , 0           }
};

enocean::T_EEP_CASE_ D2050X_CASE5 = { D2050X_CMD5,"CMD 5 - Set parameters","This command sets one or multiple configuration parameters of the actuator. When a parameter value is set to  -> no change  this parameter will not be modified. The VERT and ROT parameters describe the duration needed by the motor for a full run of the blind, or for a complete turn of the slats, respectively. They have to be measured on site and assigned to the actuator." } ;// Index of field
#define D2050X_CMD5_VERT       0
#define D2050X_CMD5_ROT        1
#define D2050X_CMD5_AA         2
#define D2050X_CMD5_CHN        3
#define D2050X_CMD5_CMD        4
#define D2050X_CMD5_NB_DATA    5
#define D2050X_CMD5_DATA_SIZE  5

enocean::T_EEP_CASE_* D2050X_CASES [] = {
&D2050X_CASE1 ,
&D2050X_CASE2 ,
&D2050X_CASE3 ,
&D2050X_CASE4 ,
&D2050X_CASE5 ,
0
};

