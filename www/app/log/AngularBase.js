define(['Base'], function (Base) {

    function AngularBase(baseParams, params) {
        baseParams.debugIsEnabled = function() { return 'true' === params.location.search().consoledebug; };
        Base.call(this, baseParams);
        this.$location = params.location;
        this.$route = params.route;
        this.$scope = params.scope;
        this.$element = params.element;
    }

    AngularBase.prototype = Object.create(Base.prototype);
    AngularBase.prototype.constructor = AngularBase;

    return AngularBase;
});
