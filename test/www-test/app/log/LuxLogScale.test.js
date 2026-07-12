const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

describe('LuxLogScale', function () {
    let LuxLogScale;

    before(function () {
        const source = fs.readFileSync(
            path.resolve(__dirname, '../../../../www/app/log/LuxLogScale.js'),
            'utf8'
        );
        vm.runInNewContext(source, {
            Math: Math,
            define: function (factory) {
                LuxLogScale = factory();
            }
        });
    });

    it('round-trips lux values including zero', function () {
        [0, 1, 10, 100, 1000, 65000].forEach(function (value) {
            const logarithmic = LuxLogScale.valueToLogarithmic(value);
            const restored = LuxLogScale.logarithmicToValue(logarithmic);
            assert.ok(Math.abs(restored - value) < 1e-9);
        });
    });

    it('configures a zero-safe logarithmic axis', function () {
        const axis = {
            dzZoom: 'full',
            update: function (options, redraw) {
                this.options = options;
                this.redraw = redraw;
                this.logarithmic = {};
            }
        };

        LuxLogScale.setAxisScale(axis, true);

        assert.strictEqual(axis.options.type, 'logarithmic');
        assert.strictEqual(axis.options.floor, 0);
        assert.strictEqual(axis.options.tickPositioner, LuxLogScale.logarithmicTickPositions);
        assert.strictEqual(axis.redraw, false);
        assert.strictEqual(axis.positiveValuesOnly, false);
        assert.strictEqual(axis.logarithmic.log2lin(0), 0);
        assert.strictEqual(axis.logarithmic.lin2log(0), 0);
        assert.strictEqual(axis.dzZoom, undefined);
    });

    it('creates readable lux ticks through the next power of ten', function () {
        const ticks = LuxLogScale.logarithmicTickPositions.call({dataMax: 65000});
        const values = Array.from(ticks, LuxLogScale.logarithmicToValue);

        assert.deepStrictEqual(values.map(Math.round), [0, 1, 10, 100, 1000, 10000, 100000]);
    });

    it('restores a linear axis', function () {
        const axis = {
            update: function (options, redraw) {
                this.options = options;
                this.redraw = redraw;
            }
        };

        LuxLogScale.setAxisScale(axis, false);

        assert.strictEqual(axis.options.type, 'linear');
        assert.strictEqual(axis.options.min, null);
        assert.strictEqual(axis.options.max, null);
        assert.strictEqual(axis.options.floor, null);
        assert.strictEqual(axis.options.tickPositioner, null);
        assert.strictEqual(axis.redraw, false);
    });
});
