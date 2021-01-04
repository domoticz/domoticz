define(function() {
    function Base(params) {
        this.$ = params.jquery;
        this.debugIsEnabled = params.debugIsEnabled;
    }

    Base.prototype.consoledebug = function(lineOrLineFunction) {
        if (this.debugIsEnabled() && console) {
            if (typeof lineOrLineFunction === 'function') {
                console.log(lineOrLineFunction());
            } else {
                console.log(lineOrLineFunction);
            }
        }
    }

    return Base;
});
