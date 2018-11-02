import {Component, OnInit} from '@angular/core';
import {interval, Subscription} from 'rxjs';
import {ApiService} from '../../_shared/_services/api.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-frontpage',
  templateUrl: './frontpage.component.html',
  styleUrls: ['./frontpage.component.css']
})
export class FrontpageComponent implements OnInit {

  roomplan = 1;
  domoticzurl = 'http://192.168.0.41:8080';
  // format: idx, value, label, comment
  PageArray = [
    ['630', 'Temp', 'itemp', 'woonkamer'],
    ['630', 'Humidity', 'ihum', 'woonkamer'],
    ['630', 'Barometer', 'astatw', 'woonkamer'],
    ['630', 'ForecastStr', 'otxt', 'woonkamer'],
    ['503', 'Temp', 'otemp', 'buiten'],
    ['503', 'Humidity', 'ohum', 'buiten'],
    ['627', 'SetPoint', 'ihi', 'room setpoint'],
    ['584', 'Usage', 'ctmep', 'elektra'],
    ['584', 'CounterToday', 'estate', 'elektra'],
    ['3', 'Rain', 'rain', 'rain'],
    ['585', 'CounterToday', 'sgas', 'gas'],
    ['411', 'UVI', 'ohi', 'uv'],
    ['613', 'Speed', 'wind', 'wind'],
    ['401', 'Status', 'ilow', 'lamp'],
  ];

  windsign = 'km/h';
  degsign = 'C';

  private refreshTimer: Subscription;

  constructor(private apiService: ApiService) {
  }

  ngOnInit() {
    this.RefreshData();
  }

  private RefreshData() {
    if (this.refreshTimer) {
      this.refreshTimer.unsubscribe();
    }

    this.apiService.callApi<any>('devices', {
      plan: this.roomplan.toString(),
      jsoncallback: '?'
    }).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        if (typeof data.WindSign !== 'undefined') {
          this.windsign = data.WindSign;
        }
        if (typeof data.TempSign !== 'undefined') {
          this.degsign = data.TempSign;
        }
        data.result.forEach((item, i) => {
          for (let ii = 0, len = this.PageArray.length; ii < len; ii++) {
            if (this.PageArray[ii][0] === item.idx) {
              const vtype = this.PageArray[ii][1];
              const vlabel = this.PageArray[ii][2];
              let vdata = item[vtype];
              if (typeof vdata === 'undefined') {
                vdata = '??';
              } else {
                vdata = new String(vdata).split(' ', 1)[0];
              }
              $('#' + vlabel).html(vdata);
            }
          }
        });
      }
    });

    this.refreshTimer = interval(10000).subscribe(() => {
      this.RefreshData();
    });
  }

}
