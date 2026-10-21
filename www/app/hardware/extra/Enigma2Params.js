
extraHWValidateParams = function (data, validators) {
	return validators["Integer"](data["Mode1"], 10, 3600, " Poll Interval");
}

extraHWInitParams = function (data) {
	if ((data["Mode1"] == "") || (data["Mode1"] == 0))
		data["Mode1"] = 60;
	$('#hardwarecontent #divextrahwparams #updatefrequencyenigma2').val(data["Mode1"]);
	$('#hardwarecontent #divextrahwparams #deepstandbyenigma2').prop('checked', data["Mode2"] == 1);
	$('#hardwarecontent #divextrahwparams #channelnameenigma2').prop('checked', data["Mode3"] == 1);
	$("#hardwarecontent #divremote #tcpaddress").val(data["Address"] ? data["Address"] : "");
	$("#hardwarecontent #divremote").show();
	$("#hardwarecontent #divlogin").show();
}

extraHWUpdateParams = function(validators) {
	var data = {};
	data["Mode1"] = $("#hardwarecontent #divextrahwparams #updatefrequencyenigma2").val();
	data["Mode2"] = $("#hardwarecontent #divextrahwparams #deepstandbyenigma2").is(":checked") ? 1 : 0;
	data["Mode3"] = $("#hardwarecontent #divextrahwparams #channelnameenigma2").is(":checked") ? 1 : 0;
	if(!extraHWValidateParams(data, validators))
		return false;
	return data;
}
