
extraHWValidateParams = function (data, validators) {
	var address = $("#hardwarecontent #tcpaddress").val();
	var port = $("#hardwarecontent #tcpport").val();

	if (!validators["String"](address, "Remote Address")) return false;
	if (!validators["Integer"](port, 1, 65535, "Port")) return false;
	if (!validators["Integer"](data["Mode3"], 1, 255, "Unit ID")) return false;
	return validators["Integer"](data["Mode1"], 5, 3600, " Poll Interval");
}

extraHWInitParams = function (data) {
	if (data["Mode1"] == "")
		data["Mode1"] = 30;
	$('#hardwarecontent #divextrahwparams #updatefrequencydaikinmodbus').val(data["Mode1"]);

	if (data["Mode2"] == "")
		data["Mode2"] = "0";
	$('#hardwarecontent #divextrahwparams #modeldaikinmodbus').val(data["Mode2"]);

	if (data["Mode3"] == "")
		data["Mode3"] = "1";
	$('#hardwarecontent #divextrahwparams #unitiddaikinmodbus').val(data["Mode3"]);

	if (data["Port"] == "" || data["Port"] == "0")
		data["Port"] = "502";

	if (data["Address"])
		$("#hardwarecontent #tcpaddress").val(data["Address"]);

	$("#hardwarecontent #divremote").show();
}

extraHWUpdateParams = function(validators) {
    var data = {};
    data["Mode1"] = $("#hardwarecontent #divextrahwparams #updatefrequencydaikinmodbus").val();
    data["Mode2"] = $("#hardwarecontent #divextrahwparams #modeldaikinmodbus").val();
    data["Mode3"] = $("#hardwarecontent #divextrahwparams #unitiddaikinmodbus").val();
    if(!extraHWValidateParams(data, validators))
        return false;
    return data;
}
