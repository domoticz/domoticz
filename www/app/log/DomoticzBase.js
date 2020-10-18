define(['AngularBase'], function (AngularBase) {
    // const AngularBase = requirejs('AngularBase');

    function DomoticzBase(baseParams, angularParams, params) {
        AngularBase.call(this, baseParams, angularParams);
        this.domoticzGlobals = params.globals;
        this.domoticzApi = params.api;
        this.domoticzDatapointApi = params.datapointApi;
    }

    DomoticzBase.prototype = Object.create(AngularBase.prototype);
    DomoticzBase.prototype.constructor = DomoticzBase;

    return DomoticzBase;
});
