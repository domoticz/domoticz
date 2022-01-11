
extraHWValidateParams = function (data, validators) {
	return validators["Integer"](data["Mode1"], 10, 3600, " Poll Interval");
}

extraHWInitParams = function (data) {
    if (data["Mode1"] == "")
        data["Mode1"] = 300;
	$('#hardwarecontent #divextrahwparams #updatefrequencydaikin').val(data["Mode1"]);
    $("#hardwarecontent #divremote").show();
    $("#hardwarecontent #divlogin").show();
}

extraHWUpdateParams = function(validators) {
    var data = {};
    data["Mode1"] = $("#hardwarecontent #divextrahwparams #updatefrequencydaikin").val();
    if(!extraHWValidateParams(data, validators))
        return false;
    return data;
}


