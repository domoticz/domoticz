define(function () {
    const logarithmBase = Math.LN10;

    function valueToLogarithmic(value) {
        return Math.log(value + 1) / logarithmBase;
    }

    function logarithmicToValue(value) {
        return Math.pow(10, value) - 1;
    }

    function logarithmicTickPositions() {
        const maximum = Math.max(this.dataMax || 0, 1);
        const maximumPower = Math.ceil(Math.log(maximum) / logarithmBase);
        const values = [0];

        for (let power = 0; power <= maximumPower; power++) {
            values.push(Math.pow(10, power));
        }

        return values.map(valueToLogarithmic);
    }

    function setAxisScale(axis, logarithmic) {
        axis.update({
            type: logarithmic ? 'logarithmic' : 'linear',
            min: null,
            max: null,
            floor: logarithmic ? 0 : null,
            tickPositioner: logarithmic ? logarithmicTickPositions : null
        }, false);

        if (logarithmic) {
            axis.logarithmic.log2lin = valueToLogarithmic;
            axis.logarithmic.lin2log = logarithmicToValue;
            axis.positiveValuesOnly = false;
        }

        axis.dzZoom = undefined;
    }

    return {
        logarithmicToValue: logarithmicToValue,
        logarithmicTickPositions: logarithmicTickPositions,
        setAxisScale: setAxisScale,
        valueToLogarithmic: valueToLogarithmic
    };
});
