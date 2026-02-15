extraHWValidateParams = function (data, validators) {
	return validators["Integer"](data["Mode1"], 10, 3600, " Poll Interval");
}

extraHWInitParams = function (data) {
    if (data["Mode1"] == "")
        data["Mode1"] = 900; // Default 15 minutes to respect API rate limits
	$('#hardwarecontent #divextrahwparams #updatefrequencytado').val(data["Mode1"]);
}

extraHWUpdateParams = function(validators) {
    var data = {};
    data["Mode1"] = $("#hardwarecontent #divextrahwparams #updatefrequencytado").val();
    if(!extraHWValidateParams(data, validators))
        return false;
    return data;
}


