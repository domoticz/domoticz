import {Camera} from '../_models/camera';
import {Device} from '../_models/device';

export class Utils {

  static GetItemBackgroundStatus(item: Device): string {
    // generate protected/timeout/lowbattery status
    let backgroundClass = 'statusNormal';
    if (item.HaveTimeout === true) {
      backgroundClass = 'statusTimeout';
    } else {
      const BatteryLevel = item.BatteryLevel;
      if (BatteryLevel !== 255) {
        if (BatteryLevel <= 10) {
          backgroundClass = 'statusLowBattery';
        } else if (item.Protected === true) {
          backgroundClass = 'statusProtected';
        }
      } else if (item.Protected === true) {
        backgroundClass = 'statusProtected';
      }
    }
    return backgroundClass;
  }

  static DisplayTrend(trend?: number): boolean {
    // 0=Unknown, 1=Stable, 2=Up, 3=Down
    if (trend) {
      if (trend > 1) {
        return true;
      }
    }
    return false;
  }

  static TrendState(trend: number): string {
    if (trend === 0) {
      return 'unk';
    }
    if (trend === 1) {
      return 'stable';
    }
    if (trend === 2) {
      return 'up';
    }
    if (trend === 3) {
      return 'down';
    }
    return 'unk';
  }

  static EvoDisplayTextMode(strstatus: string) {
    if (strstatus === 'Auto') {// FIXME better way to convert?
      strstatus = 'Normal';
    } else if (strstatus === 'AutoWithEco') {// FIXME better way to convert?
      strstatus = 'Economy';
    } else if (strstatus === 'DayOff') {// FIXME better way to convert?
      strstatus = 'Day Off';
    } else if (strstatus === 'HeatingOff') {// FIXME better way to convert?
      strstatus = 'Heating Off';
    }
    return strstatus;
  }

  static checkLength(val: string, min: number, max: number) {
    if (val.length > max || val.length < min) {
      return false;
    } else {
      return true;
    }
  }

  static GetPrevDateFromString(s: string): number {
    return Date.UTC(
      parseInt(s.substring(0, 4), 10) + 1,
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10));
  }

  static GetUTCFromString(s: string): number {
    return Date.UTC(
      parseInt(s.substring(0, 4), 10),
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10),
      parseInt(s.substring(11, 13), 10),
      parseInt(s.substring(14, 16), 10),
      0
    );
  }

  static GetDateFromString(s: string): number {
    return Date.UTC(
      parseInt(s.substring(0, 4), 10),
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10));
  }

  static CalculateTrendLine(data): { x0: number, y0: number, x1: number, y1: number } {
    // function taken from jquery.flot.trendline.js
    let ii = 0, x, y, x0, x1, y0, y1, dx,
      m = 0, b = 0, cs, ns,
      n = data.length, Sx = 0, Sy = 0, Sxy = 0, Sx2 = 0, S2x = 0;

    // Not enough data
    if (n < 2) {
      return;
    }

    // Do math stuff
    for (ii; ii < data.length; ii++) {
      x = data[ii][0];
      y = data[ii][1];
      Sx += x;
      Sy += y;
      Sxy += (x * y);
      Sx2 += (x * x);
    }
    // Calculate slope and intercept
    m = (n * Sx2 - S2x) !== 0 ? (n * Sxy - Sx * Sy) / (n * Sx2 - Sx * Sx) : 0;
    b = (Sy - m * Sx) / n;

    // Calculate minimal coordinates to draw the trendline
    dx = 0; // parseFloat(data[1][0]) - parseFloat(data[0][0]);
    x0 = parseFloat(data[0][0]) - dx;
    y0 = m * x0 + b;
    x1 = parseFloat(data[ii - 1][0]) + dx;
    y1 = m * x1 + b;

    const dReturn = {x0, y0, x1, y1};
    return dReturn;
  }

  static b64DecodeUnicode(str: string) {
    try {
      return decodeURIComponent(Array.prototype.map.call(atob(str), (c: any) => {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
      }).join(''));
    } catch (e) {
      // Pff fallback
      return atob(str);
    }
  }

  static b64EncodeUnicode(str: string) {
    return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g, (match: string, p1: any) => {
      return String.fromCharCode(parseInt(p1, 16));
    }));
  }

  static GetUTCFromStringSec(s: string) {
    return Date.UTC(
      parseInt(s.substring(0, 4), 10),
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10),
      parseInt(s.substring(11, 13), 10),
      parseInt(s.substring(14, 16), 10),
      parseInt(s.substring(17, 19), 10)
    );
  }

  static addLeadingZeros(n: number | string, length: number) {
    const str = n.toString();
    let zeros = '';
    for (let i = length - str.length; i > 0; i--) {
      zeros += '0';
    }
    zeros += str;
    return zeros;
  }

  static isLED(SubType: string): boolean {
    return (SubType.indexOf('RGB') >= 0 || SubType.indexOf('WW') >= 0);
  }

  static getLEDType(SubType: string): LedType {
    const LEDType: LedType = {
      bIsLED: false,
      bHasRGB: false,
      bHasWhite: false,
      bHasTemperature: false,
      bHasCustom: false
    };
    LEDType.bIsLED = (SubType.indexOf('RGB') >= 0 || SubType.indexOf('WW') >= 0);
    LEDType.bHasRGB = (SubType.indexOf('RGB') >= 0);
    LEDType.bHasWhite = (SubType.indexOf('W') >= 0);
    LEDType.bHasTemperature = (SubType.indexOf('WW') >= 0);
    LEDType.bHasCustom = (SubType.indexOf('RGBWZ') >= 0 || SubType.indexOf('RGBWWZ') >= 0);

    return LEDType;
  }

  static GetLightStatusText(item: Device): string {
    if (item.SubType === 'Evohome') {
      return Utils.EvoDisplayTextMode(item.Status);
    } else if (item.SwitchType === 'Selector') {
      return Utils.b64DecodeUnicode(item.LevelNames).split('|')[(item.LevelInt / 10)];
    } else {
      return item.Status;
    }
  }

  static strPad(i: string, l: number, s?: string): string {
    let o = i.toString();
    if (!s) {
      s = '0';
    }
    while (o.length < l) {
      o = s + o;
    }
    return o;
  }

  static setCharAt(str: string, idx: number, chr: string): string {
    if (idx > str.length - 1) {
      return str.toString();
    } else {
      return str.substr(0, idx) + chr + str.substr(idx + 1);
    }
  }

  // tslint:disable:no-bitwise
  static decodeBase64(s: string): string {
    const e = {};
    let i;
    let b = 0;
    let c;
    let x;
    let l = 0;
    let a;
    let r = '';
    const w = String.fromCharCode;
    const L = s.length;
    const A = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    for (i = 0; i < 64; i++) {
      e[A.charAt(i)] = i;
    }
    for (x = 0; x < L; x++) {
      c = e[s.charAt(x)];
      b = (b << 6) + c;
      l += 6;
      while (l >= 8) {
        ((a = (b >>> (l -= 8)) & 0xff) || (x < (L - 2))) && (r += w(a));
      }
    }
    return r;
  }

  static cursorhand(): void {
    document.body.style.cursor = 'pointer';
  }

  static cursordefault(): void {
    document.body.style.cursor = 'default';
  }

  static GenerateCamImageURL(cam: Camera): string {
    let feedsrc;
    if (cam.Protocol === 0) {
      feedsrc = 'http://';
    } else {
      feedsrc = 'https://';
    }
    const bHaveUPinURL = (cam.ImageURL.indexOf('#USERNAME') !== -1) || (cam.ImageURL.indexOf('#PASSWORD') !== -1);
    if (!bHaveUPinURL) {
      if (cam.Username !== '') {
        feedsrc += cam.Username + ':' + cam.Password + '@';
      }
    }
    feedsrc += cam.Address;
    if (cam.Port !== 80) {
      feedsrc += ':' + cam.Port;
    }
    feedsrc += '/' + cam.ImageURL;
    if (bHaveUPinURL) {
      feedsrc = feedsrc.replace('#USERNAME', cam.Username);
      feedsrc = feedsrc.replace('#PASSWORD', cam.Password);
    }
    return feedsrc;
  }

  // Some helper for browser detection
  static matchua(ua: string): { browser: string, version: string } {
    ua = ua.toLowerCase();

    const match = /(chrome)[ \/]([\w.]+)/.exec(ua) ||
      /(webkit)[ \/]([\w.]+)/.exec(ua) ||
      /(opera)(?:.*version|)[ \/]([\w.]+)/.exec(ua) ||
      /(msie) ([\w.]+)/.exec(ua) ||
      ua.indexOf('compatible') < 0 && /(mozilla)(?:.*? rv:([\w.]+)|)/.exec(ua) ||
      [];

    return {
      browser: match[1] || '',
      version: match[2] || '0'
    };
  }

  // Convert time format taking account the time zone offset. Improved version of toISOString() function.
  // Example from "Wed Apr 01 2020 17:00:00 GMT+0100 (British Summer Time)" to "2020-04-01T17:00:00.000Z"
  static ConvertTimeWithTimeZoneOffset(tUnit: Date): string {
    const tzoffset = (new Date(tUnit)).getTimezoneOffset() * 60000; // offset in millisecondos
    const tUntilWithTimeZoneOffset = (new Date(tUnit.getTime() - tzoffset)).toISOString().slice(0, -1) + 'Z';
    return tUntilWithTimeZoneOffset;
  }


}

export class LedType {
  bIsLED: boolean;
  bHasRGB: boolean;
  bHasWhite: boolean;
  bHasTemperature: boolean;
  bHasCustom: boolean;
}
