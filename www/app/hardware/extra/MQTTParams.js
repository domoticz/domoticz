
extraHWValidateParams = function (data, validators) {

	if(!data["extra"])
		return true;

	var p = data["extra"].split(";");
	var topin = "";
	var topout = "";
	var topdisc = "";

	if (p.length > 1)
		topin = p[1];
	if (p.length > 2)
		topout = p[2];

	if(window.__hwfnparam == 3)
	{
	    if (p.length > 3)
		{
	    	topdisc = p[3];
    	}
		return validators["String"](topdisc, "Auto Discovery Prefix")
		    && validators["MQTTTopic"](topdisc, "Auto Discovery Prefix");
	}

	return validators["MQTTTopic"](topin, "Topic in Prefix")
        && validators["MQTTTopic"](topout, "Topic out Prefix");
}

extraHWInitParams = function(data) {

	$("#hardwarecontent #hardwareparamsmqtt #filename").val("");
	$("#hardwarecontent #hardwareparamsmqtt #multidomonodesync").val("");
	$("#hardwarecontent #divextrahwparams #mqtttopicin").val("");
	$("#hardwarecontent #divextrahwparams #mqtttopicout").val("");
	$("#hardwarecontent #divextrahwparams #mqttdiscoveryprefix").val("");

	if (!data["Extra"])
	{
		data["Extra"] = ";domoticz/in;domoticz/out";
		if (window.__hwfnparam == 3)
			data["Extra"] += ";homeassistant";
	}

	if (data["Mode1"] === '')
		data["Mode1"] = 1;

	if (data["Mode2"] === '')
		data["Mode2"] = 2;

	if (data["Mode3"] === '')
		data["Mode3"] = 1;

	if (data["Mode4"] === '')
		data["Mode4"] = 0;

	if (!data["Port"])
		data["Port"] = 1883;

	// Break out any possible topic prefix pieces.
	var CAfilenameParts = data["Extra"].split(";");

	if (CAfilenameParts.length > 0)
		$("#hardwarecontent #hardwareparamsmqtt #filename").val(CAfilenameParts[0]);
	if (CAfilenameParts.length > 1)
		$("#hardwarecontent #hardwareparamsmqtt #mqtttopicin").val(CAfilenameParts[1]);
	if (CAfilenameParts.length > 2)
		$("#hardwarecontent #hardwareparamsmqtt #mqtttopicout").val(CAfilenameParts[2]);

	if(window.__hwfnparam == 3)
	{
    	if (CAfilenameParts.length > 3)
      		$("#hardwarecontent #hardwareparamsmqtt #mqttdiscoveryprefix").val(CAfilenameParts[3]);
	}

	$("#hardwarecontent #divextrahwparams #hardwareparamsmqtt #combotopicselect").val(data["Mode1"]);
	$("#hardwarecontent #hardwareparamsmqtt #combotlsversion").val(data["Mode2"]);
	$("#hardwarecontent #hardwareparamsmqtt #combopreventloop").val(data["Mode3"]);
	$("#hardwarecontent #hardwareparamsmqtt #multidomonodesync").prop("checked", data["Mode4"] == 1)
	$("#hardwarecontent #divremote").show();
	$("#hardwarecontent #divlogin").show();
	$("#hardwarecontent #hardwareparamsmqtt #multi_domo_node_sync").hide();

	if(window.__hwfnparam == 0)
		$("#hardwarecontent #divextrahwparams #mqtt_publish").show();
	else
		$("#hardwarecontent #divextrahwparams #mqtt_publish").hide();

	if(window.__hwfnparam == 3) {
		//Auto Discovery
		$("#hardwarecontent #divextrahwparams #mqtt_topic_in_out").hide();
		$("#hardwarecontent #divextrahwparams #mqtt_preventloop").hide();
		$("#hardwarecontent #divextrahwparams #mqtt_auto_dicovery").show();
    } else if( window.__hwfnparam == 4 ) {
		// RFLink Gateway MQTT
		$("#hardwarecontent #divextrahwparams #mqtt_topic_in_out").hide();
		$("#hardwarecontent #divextrahwparams #mqtt_preventloop").hide();
		$("#hardwarecontent #divextrahwparams #mqtt_auto_dicovery").hide();
		$("#hardwarecontent #hardwareparamsmqtt #multi_domo_node_sync").show();
	} else {
		$("#hardwarecontent #divextrahwparams #mqtt_preventloop").show();
		$("#hardwarecontent #divextrahwparams #mqtt_topic_in_out").show();
		$("#hardwarecontent #divextrahwparams #mqtt_auto_dicovery").hide();
	}
}

extraHWUpdateParams = function(validators) {
	var data = {};
	var mqtttopicin = $("#hardwarecontent #divextrahwparams #mqtttopicin").val().trim();
	var mqtttopicout = $("#hardwarecontent #divextrahwparams #mqtttopicout").val().trim();
	var mqttdiscoveryprefix = $("#hardwarecontent #divextrahwparams #mqttdiscoveryprefix").val().trim();

	data["extra"] = $("#hardwarecontent #divextrahwparams #filename").val().trim();
	data["extra"] += ";" + mqtttopicin + ";" + mqtttopicout;

	if(window.__hwfnparam == 3)
	{
    	data["extra"] += ";" + mqttdiscoveryprefix;
	}

	data["Mode1"] = $("#hardwarecontent #divextrahwparams #combotopicselect").val();
	data["Mode2"] = $("#hardwarecontent #divextrahwparams #combotlsversion").val();
	data["Mode3"] = $("#hardwarecontent #divextrahwparams #combopreventloop").val();
	data["Mode4"] = $("#hardwarecontent #hardwareparamsmqtt #multidomonodesync").prop("checked") ? 1 : 0;

	if(!extraHWValidateParams(data, validators))
		return false;

	return data;
}
