import {Component, Inject, OnInit} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {DeviceService} from '../../_shared/_services/device.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Device} from '../../_shared/_models/device';
import {monthNames as monthNamesConst} from '../../_shared/_constants/months';

@Component({
  selector: 'dz-device-report',
  templateUrl: './device-report.component.html',
  styleUrls: ['./device-report.component.css']
})
export class DeviceReportComponent implements OnInit {

  deviceIdx: string;
  device: Device;

  selectedYear: number;
  selectedMonth: number;
  isMonthView: boolean;

  monthNames = monthNamesConst;

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private deviceService: DeviceService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) { }

  ngOnInit() {
    this.deviceIdx = this.route.snapshot.paramMap.get('idx');

    this.deviceService.getDeviceInfo(this.deviceIdx).subscribe(device => {
      this.device = device;
    });

    this.route.paramMap.subscribe(paramMap => {
      if (paramMap.has('year')) {
        this.selectedYear = Number(paramMap.get('year'));
      } else {
        this.selectedYear = (new Date()).getFullYear();
      }

      if (paramMap.has('month')) {
        this.selectedMonth = Number(paramMap.get('month'));
      } else {
        this.selectedMonth = undefined;
      }

      this.isMonthView = this.selectedMonth && this.selectedMonth > 0;
    });
  }

  get yearsOptions(): Array<number> {
    const currentYear = (new Date()).getFullYear();
    const years = [];

    for (let i = 2012; i <= currentYear; i++) {
      years.push(i);
    }

    return years;
  }

  isTemperatureReport(): boolean {
    if (this.device.Type === 'Heating') {
      return (this.device.SubType === 'Zone');
    }

    // This goes wrong (when we also use this log call from the weather tab), for wind sensors
    // as this is placed in weather and temperature, we might have to set a parameter in the url
    // for now, we assume it is a temperature
    return (/Temp|Thermostat|Humidity|Radiator|Wind/i).test(this.device.Type);
  }

  isCounterReport(): boolean {
    if (!this.device) {
      return undefined;
    }
    return ['Power', 'Energy', 'RFXMeter'].includes(this.device.Type)
      || this.isOnlyUsage()
      || ['kWh'].includes(this.device.SubType)
      || ['YouLess counter'].includes(this.device.SubType)
      || ['Counter Incremental'].includes(this.device.SubType)
      || (this.device.Type === 'P1 Smart Meter' && this.device.SubType !== 'Energy');
  }

  isOnlyUsage() {
    if (!this.device) {
      return undefined;
    }
    return ['Managed Counter'].includes(this.device.SubType);
  }

  isEnergyMultiCounterReport(): boolean {
    if (!this.device) {
      return undefined;
    }

    return (this.device.Type === 'P1 Smart Meter' && this.device.SubType === 'Energy');
  }

  isNoReport(): boolean {
    if (!this.device) {
      return undefined;
    }

    return !this.isTemperatureReport() && !this.isCounterReport() && !this.isEnergyMultiCounterReport();
  }

  selectYear() {
    this.router.navigate(['/Devices', this.deviceIdx, 'Report', this.selectedYear]);
  }

}
