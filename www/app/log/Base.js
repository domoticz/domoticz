define(function() {
    function Base(params) {
        this.$ = params.jquery;
        this.debugIsEnabled = params.debugIsEnabled;
    }

    Base.prototype.consoledebug = function(line) {
        if (this.debugIsEnabled() && console) {
            console.log(line);
        }
    }

    return Base;
});
