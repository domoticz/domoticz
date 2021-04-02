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

    Base.dateToString = function(input) {
        if (input === undefined) {
            return 'undefined';
        }
        if (input === null) {
            return 'null';
        }
        if (input === Infinity) {
            return 'Infinity';
        }
        if (input === -Infinity) {
            return '-Infinity';
        }
        if (typeof input === 'string' || typeof input === 'number') {
            date = new Date(input);
        } else {
            date = input;
        }
        return date.getFullYear() + '-'+ pad2(date.getMonth()) + '-'+ pad2(date.getDate()) + 'T' + pad2(date.getHours()) + ':'+ pad2(date.getMinutes()) + ':' + pad2(date.getSeconds()) + '.'+ pad3(date.getMilliseconds());

        function pad2(n) {
            return (n < 10 ? '0' : '') + n;
        }

        function pad3(n) {
            return (n < 100 ? '0' : '') + pad2(n);
        }
    }

    return Base;
});
