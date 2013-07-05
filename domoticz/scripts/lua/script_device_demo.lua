-- demo device script
-- script names have three name components: script_trigger_name.lu
-- trigger can be 'time' or 'device', name can be any string
-- domoticz will execute all time and device triggers when the relevant trigger occurs
-- ingests tables: devicestatus,svalues,nvalues
-- device status is a three item array: devicestatus['id'], devicestatus['nvalue'] and devicestatus['svalue'] 
-- arrays svalues and nvalues contain complete value lists for all enabled devices

// the example below checks if the incoming device notification is about device with id 1. If this device is turned on then also turn device 2 on.

commandArray = {}
if (devicestatus['id'] == 1 and devicestatus['nvalue'] == 1) then
	commandArray[2]=1;
end
return commandArray
