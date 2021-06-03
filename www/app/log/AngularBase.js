define(['Base'], function (Base) {

    function AngularBase(baseParams, params) {
        baseParams.consoledebugIsEnabled = function() { return 'true' === params.location.search().consoledebug; };
        baseParams.windowdebugIsEnabled = function() { return 'true' === params.location.search().windowdebug; };
        Base.call(this, baseParams);
        this.$location = params.location;
        this.$route = params.route;
        this.$scope = params.scope;
        this.$timeout = params.timeout;
        this.$element = params.element;
    }

    AngularBase.prototype = Object.create(Base.prototype);
    AngularBase.prototype.constructor = AngularBase;

    return AngularBase;
});
