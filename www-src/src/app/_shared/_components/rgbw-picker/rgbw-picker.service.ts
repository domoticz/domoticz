import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { Injectable, Inject, ElementRef } from '@angular/core';
import { Utils } from 'src/app/_shared/_utils/utils';
import { PermissionService } from '../../_services/permission.service';
import { NotificationService } from '../../_services/notification.service';
import { ApiService } from '../../_services/api.service';
import { timer, Subscription } from 'rxjs';

// FIXME proper declaration
declare var $: any;

// FIXME clean all this stuff
@Injectable()
export class RgbwPickerService {

  public setColValue: Subscription;

  constructor(
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private apiService: ApiService
  ) { }

  public ShowRGBWPicker(containerRef: ElementRef, idx: any, Protected: number,
    MaxDimLevel: number, LevelInt: number, colorJSON: string, iSubType: string, iDimmerType: any,
    callback?: (devIdx, JSONColor, dimlevel) => any) {

    const _this = this;

    let color: any = {};
    const devIdx = idx;
    const SubType = iSubType;
    const DimmerType = iDimmerType;
    const LEDType = Utils.getLEDType(SubType);

    try {
      color = JSON.parse(colorJSON);
    } catch (e) {
      // forget about it :)
    }
    let colorPickerMode = 'color'; // Default

    // TODO: A little bit hackish, maybe extend the wheelColorPicker instead..
    $('#popup_picker', containerRef.nativeElement)[0].getJSONColor = function () {
      let _colorJSON = ''; // Empty string, intentionally illegal JSON
      let fcolor = $(this).wheelColorPicker('getColor'); // Colors as floats 0..1
      if (colorPickerMode === 'white') {
        _colorJSON = JSON.stringify({
          m: 1,
          t: 0,
          r: 0,
          g: 0,
          b: 0,
          cw: 255,
          ww: 255
        });
      }
      if (colorPickerMode === 'temperature') {
        _colorJSON = JSON.stringify({
          m: 2,
          t: Math.round(fcolor.t * 255),
          r: 0,
          g: 0,
          b: 0,
          cw: Math.round((1 - fcolor.t) * 255),
          ww: Math.round(fcolor.t * 255)
        });
      } else if (colorPickerMode === 'color') {
        // Set value to 1 in color mode
        $(this).wheelColorPicker('setHsv', fcolor.h, fcolor.s, 1);
        fcolor = $(this).wheelColorPicker('getColor'); // Colors as floats 0..1
        _colorJSON = JSON.stringify({
          m: 3,
          t: 0,
          r: Math.round(fcolor.r * 255),
          g: Math.round(fcolor.g * 255),
          b: Math.round(fcolor.b * 255),
          cw: 0,
          ww: 0
        });
      } else if (colorPickerMode === 'customw') {
        _colorJSON = JSON.stringify({
          m: 4,
          t: 0,
          r: Math.round(fcolor.r * 255),
          g: Math.round(fcolor.g * 255),
          b: Math.round(fcolor.b * 255),
          cw: Math.round(fcolor.w * 255),
          ww: Math.round(fcolor.w * 255)
        });
      } else if (colorPickerMode === 'customww') {
        _colorJSON = JSON.stringify({
          m: 4,
          t: Math.round(fcolor.t * 255),
          r: Math.round(fcolor.r * 255),
          g: Math.round(fcolor.g * 255),
          b: Math.round(fcolor.b * 255),
          cw: Math.round(fcolor.w * (1 - fcolor.t) * 255),
          ww: Math.round(fcolor.w * fcolor.t * 255)
        });
      }
      return _colorJSON;
    };

    function UpdateColorPicker(mode) {
      colorPickerMode = mode;
      if (mode === 'color') {
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: 'wm', preserveWheel: true });
      } else if (mode === 'color_no_master') {
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: 'w', preserveWheel: true });
      } else if (mode === 'white') {
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: 'm', preserveWheel: true });
      } else if (mode === 'white_no_master') {
        // TODO: Silly, nothing to show!
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: '', preserveWheel: true });
      } else if (mode === 'temperature') {
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: 'xm' });
      } else if (mode === 'temperature_no_master') {
        // TODO: Silly, nothing to show!
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: '' });
      } else if (mode === 'customw') {
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: 'wvlm', preserveWheel: false });
      } else if (mode === 'customww') {
        $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setOptions', { sliders: 'wvklm', preserveWheel: false });
      }

      $('.pickermodergb', containerRef.nativeElement).hide();
      $('.pickermodewhite', containerRef.nativeElement).hide();
      $('.pickermodetemp', containerRef.nativeElement).hide();
      $('.pickermodecustomw', containerRef.nativeElement).hide();
      $('.pickermodecustomww', containerRef.nativeElement).hide();
      // Show buttons for choosing input mode
      let supportedModes = 0;
      if (LEDType.bHasRGB) { supportedModes++; }
      if (LEDType.bHasWhite && !LEDType.bHasTemperature && DimmerType !== 'rel') { supportedModes++; }
      if (LEDType.bHasTemperature) { supportedModes++; }
      if (LEDType.bHasCustom && !LEDType.bHasTemperature) { supportedModes++; }
      if (LEDType.bHasCustom && LEDType.bHasTemperature) { supportedModes++; }
      if (supportedModes > 1) {
        if (LEDType.bHasRGB) {
          if (mode === 'color' || mode === 'color_no_master') {
            $('.pickermodergb.selected', containerRef.nativeElement).show();
          } else {
            $('.pickermodergb.unselected', containerRef.nativeElement).show();
          }
        }
        if (LEDType.bHasWhite && !LEDType.bHasTemperature && DimmerType !== 'rel') {
          if (mode === 'white' || mode === 'white_no_master') {
            $('.pickermodewhite.selected', containerRef.nativeElement).show();
          } else {
            $('.pickermodewhite.unselected', containerRef.nativeElement).show();
          }
        }
        if (LEDType.bHasTemperature && DimmerType !== 'rel') {
          if (mode === 'temperature' || mode === 'temperature_no_master') {
            $('.pickermodetemp.selected', containerRef.nativeElement).show();
          } else {
            $('.pickermodetemp.unselected', containerRef.nativeElement).show();
          }
        }
        if (LEDType.bHasCustom && !LEDType.bHasTemperature) {
          if (mode === 'customw') {
            $('.pickermodecustomw.selected', containerRef.nativeElement).show();
          } else {
            $('.pickermodecustomw.unselected', containerRef.nativeElement).show();
          }
        }
        if (LEDType.bHasCustom && LEDType.bHasTemperature) {
          if (mode === 'customww') {
            $('.pickermodecustomww.selected', containerRef.nativeElement).show();
          } else {
            $('.pickermodecustomww.unselected', containerRef.nativeElement).show();
          }
        }
      }

      $('.pickerrgbcolorrow', containerRef.nativeElement).hide();
      // Show RGB hex input
      if (LEDType.bHasRGB) {
        if (mode === 'color' || mode === 'color_no_master') {
          $('.pickerrgbcolorrow', containerRef.nativeElement).show();
        }
      }

      $('#popup_picker', containerRef.nativeElement).wheelColorPicker('refreshWidget');
      $('#popup_picker', containerRef.nativeElement).wheelColorPicker('updateSliders');
      $('#popup_picker', containerRef.nativeElement).wheelColorPicker('redrawSliders');
    }

    /**enum ColorMode {
      ColorModeNone = 0, // Illegal
      ColorModeWhite,    // White. Valid fields: none
      ColorModeTemp,     // White with color temperature. Valid fields: t
      ColorModeRGB,      // Color. Valid fields: r, g, b
      ColorModeCustom,   // Custom (color + white). Valid fields: r, g, b, cw, ww, depending on device capabilities
    };*/

    let color_m = (color.m == null) ? 3 : color.m; // Default to 3: ColorModeRGB

    if (color_m !== 1 && color_m !== 2 && color_m !== 3 && color_m !== 4) { color_m = 3; } // Default to RGB if not valid
    if (color_m === 4 && !LEDType.bHasCustom) { color_m = 3; } // Default to RGB if light does not support custom color
    if (color_m === 1 && !LEDType.bHasWhite) { color_m = 3; } // Default to RGB if light does not support white
    if (color_m === 2 && !LEDType.bHasTemperature) { color_m = 3; } // Default to RGB if light does not support temperature
    if (color_m === 3 && !LEDType.bHasRGB) {
      if (LEDType.bHasTemperature) {
        // Default to temperature if light does not support RGB but does support temperature
        color_m = 2;
      } else {
        // Default to white if light does not support either RGB or temperature (in this case just a dimmer slider should be shown though)
        color_m = 1;
      }
    }

    let color_t = 128;
    let color_cw = 128;
    let color_ww = 255 - color_cw;
    let color_r = 255;
    let color_g = 255;
    let color_b = 255;

    if (color_m === 1) {
      // Nothing..
    }
    if (color_m === 2) {
      color_t = (color.t == null) ? 128 : color.t;
      color_cw = (color.cw == null) ? 128 : color.cw;
      color_ww = (color.ww == null) ? 255 - color_cw : color.ww;
    }
    if (color_m === 3) {
      color_r = (color.r == null) ? 255 : color.r;
      color_g = (color.g == null) ? 255 : color.g;
      color_b = (color.b == null) ? 255 : color.b;
    }
    if (color_m === 4) {
      color_t = (color.t == null) ? 128 : color.t;
      color_cw = (color.cw == null) ? 128 : color.cw;
      color_ww = (color.ww == null) ? 255 - color_cw : color.ww;
      color_r = (color.r == null) ? 255 : color.r;
      color_g = (color.g == null) ? 255 : color.g;
      color_b = (color.b == null) ? 255 : color.b;
    }

    // TODO: white_no_master and temperature_no_master are meaningless, remove
    if (color_m === 1) { // White mode
      colorPickerMode = DimmerType !== 'rel' ? 'white' : 'white_no_master';
    }
    if (color_m === 2) { // Color temperature mode
      colorPickerMode = DimmerType !== 'rel' ? 'temperature' : 'temperature_no_master';
    } else if (color_m === 3) { // Color  mode
      colorPickerMode = DimmerType !== 'rel' ? 'color' : 'color_no_master';
    } else if (color_m === 4) { // Custom  mode
      colorPickerMode = 'customw';
      if (LEDType.bHasTemperature) {
        colorPickerMode = 'customww';
      }
    }

    $('.pickermodergb', containerRef.nativeElement).off().click(function () {
      UpdateColorPicker(DimmerType !== 'rel' ? 'color' : 'color_no_master');
    });
    $('.pickermodewhite', containerRef.nativeElement).off().click(function () {
      UpdateColorPicker(DimmerType !== 'rel' ? 'white' : 'white_no_master');
    });
    $('.pickermodetemp', containerRef.nativeElement).off().click(function () {
      UpdateColorPicker(DimmerType !== 'rel' ? 'temperature' : 'temperature_no_master');
    });
    $('.pickermodecustomw', containerRef.nativeElement).off().click(function () {
      UpdateColorPicker('customw');
    });
    $('.pickermodecustomww', containerRef.nativeElement).off().click(function () {
      UpdateColorPicker('customww');
    });

    $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setTemperature', color_t / 255);
    $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setWhite', color_cw / 255 + color_ww / 255);
    $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setRgb', color_r / 255, color_g / 255, color_b / 255);
    $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setMaster', LevelInt / MaxDimLevel);

    const rgbhex = $('#popup_picker', containerRef.nativeElement).wheelColorPicker('getValue', 'hex').toUpperCase();
    $('.pickerrgbcolorinput', containerRef.nativeElement).val(rgbhex);

    // Update color picker controls
    UpdateColorPicker(colorPickerMode);

    $('#popup_picker', containerRef.nativeElement).off('slidermove sliderup').on('slidermove sliderup', function () {
      if (_this.setColValue) {
        _this.setColValue.unsubscribe();
      }

      const _color = $(this).wheelColorPicker('getColor');
      const _rgbhex = $(this).wheelColorPicker('getValue', 'hex').toUpperCase();
      const dimlevel = Math.round((_color.m * 99) + 1); // 1..100
      const JSONColor = $('#popup_picker', containerRef.nativeElement)[0].getJSONColor();
      // TODO: Rate limit instead of debounce
      _this.setColValue = timer(400).subscribe(() => {
        const fn = callback || _this.SetColValue.bind(_this);
        fn(devIdx, JSONColor, dimlevel);
      });
      $('.pickerrgbcolorinput', containerRef.nativeElement).val(_rgbhex);
    });
    $('.pickerrgbcolorinput', containerRef.nativeElement).off('input').on('input', function () {
      $('#popup_picker', containerRef.nativeElement).wheelColorPicker('setValue', this.value);
    });
  }

  SetColValue(idx, color, brightness) {
    if (this.setColValue) {
      this.setColValue.unsubscribe();
    }

    if (this.permissionService.hasPermission('User')) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return;
    }
    this.apiService.callApi('command', { param: 'setcolbrightnessvalue', idx: idx, color: color, brightness: brightness })
      .subscribe(() => {
        // Nothing
      });
  }

  public updateSliders(containerRef: ElementRef) {
    $('#popup_picker', containerRef.nativeElement).wheelColorPicker('updateSliders');
  }

  public redrawSliders(containerRef: ElementRef) {
    $('#popup_picker', containerRef.nativeElement).wheelColorPicker('redrawSliders');
  }

}
