import {Utils} from 'src/app/_shared/_utils/utils';
import {GraphTempPoint} from '../../_shared/_models/graphdata';
import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import * as Highcharts from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ConfigService} from '../../_shared/_services/config.service';
import {DeviceService} from '../../_shared/_services/device.service';
import {GraphService} from '../../_shared/_services/graph.service';
import * as dateFormat from 'dateformat';
import {GraphData} from 'src/app/_shared/_models/graphdata';
import {Device} from '../../_shared/_models/device';
import {ChartService} from '../../_shared/_services/chart.service';

@Component({
  selector: 'dz-custom-temp-log',
  templateUrl: './custom-temp-log.component.html',
  styleUrls: ['./custom-temp-log.component.css']
})
export class CustomTempLogComponent implements OnInit, AfterViewInit {

  @ViewChild('customgraph', {static: false}) customgraph: ElementRef;

  devices: Array<DeviceCheck> = [];

  customgraphtype: string;

  from: string;
  to: string;

  graphTemp = true;
  graphChill = false;
  graphHum = false;
  graphBaro = false;
  graphDew = false;
  graphSet = false;

  chart: Highcharts.Chart;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService,
    private graphService: GraphService) {
  }

  ngOnInit() {
    this.customgraphtype = '1';

    const d = new Date();
    this.to = dateFormat(d, 'yyyy-mm-dd');

    d.setDate(d.getDate() - 7);
    this.from = dateFormat(d, 'yyyy-mm-dd');

    this.SelectGraphDevices();
  }

  ngAfterViewInit() {

    const _this = this;

    const chartOptions: Highcharts.Options = {
      chart: {
        type: 'line',
        zoomType: 'x',
        alignTicks: false,

        events: {
          load: function () {
          }
        }
      },
      colors: ['#FF99CC', '#FFCC99', '#FFFF99', '#CCFFCC', '#CCFFFF', '#99CCFF', '#CC99FF', '#FFFFFF',
        '#9999FF', '#993366', '#FFFFCC', '#CCFFFF', '#660066', '#FF8080', '#0066CC', '#CCCCFF',
        '#000080', '#FF00FF', '#FFFF00', '#00FFFF', '#800080', '#800000', '#008080', '#0000FF',
        '#00C0C0', '#993300', '#333300', '#003300', '#003366', '#000080', '#333399', '#333333',
        '#800000', '#FF6600', '#808000', '#008000', '#008080', '#0000FF', '#666699', '#808080',
        '#FF0000', '#FF9900', '#99CC00', '#339966', '#33CCCC', '#3366FF', '#800080', '#969696',
        '#FF00FF', '#FFCC00', '#FFFF00', '#00FF00', '#00FFFF', '#00CCFF', '#993366', '#C0C0C0'],
      loading: {
        hideDuration: 1000,
        showDuration: 1000
      },
      title: {
        text: this.translationService.t('Custom Temperature Graph')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: [{ // temp label
        labels: {
          formatter: function () {
            return this.value + '\u00B0 ' + _this.configService.config.TempSign;
          },
          style: {
            color: 'white'
          }
        },
        title: {
          text: 'degrees Celsius',
          style: {
            color: 'white'
          }
        },
        showEmpty: false
      }, { // humidity label
        labels: {
          formatter: function () {
            return this.value + '%';
          },
          style: {
            color: 'white'
          }
        },
        title: {
          text: this.translationService.t('Humidity') + ' %',
          style: {
            color: 'white'
          }
        },
        showEmpty: false
      }, { // pressure label
        labels: {
          formatter: function () {
            return this.value.toString();
          },
          style: {
            color: 'white'
          }
        },
        title: {
          text: this.translationService.t('Pressure') + ' (hPa)',
          style: {
            color: 'white'
          }
        },
        showEmpty: false
      }],
      tooltip: {
        formatter: function () {
          let unit = '';
          const baseName = this.series.name.split(':')[1];
          if (baseName === _this.translationService.t('Humidity')) {
            unit = '%';
          } else if (baseName === _this.translationService.t('Barometer')) {
            unit = ' hPa';
          } else {
            unit = '\u00B0 ' + _this.configService.config.TempSign;
          }
          return _this.translationService.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' +
            Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + '<br/>' + this.series.name + ': ' + this.y + unit;
        }
      },
      legend: {
        enabled: true
      },
      plotOptions: {
        line: {
          lineWidth: 3,
          states: {
            hover: {
              lineWidth: 3
            }
          },
          marker: {
            enabled: false,
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            },
            symbol: 'circle'
          }
        }
      }
    };

    ChartService.setDeviceIdx('custom');
    this.chart = Highcharts.chart(this.customgraph.nativeElement, chartOptions);

  }

  private SelectGraphDevices() {
    this.deviceService.getDevices('temp', true, 'Name').subscribe(response => {
      this.devices = response.result.map((item: Device) => {
        return {
          item: item,
          checked: false
        };
      });
    });
  }

  ClearCustomGraph() {
    this.devices.filter(d => d.checked === true).forEach(d => {
      this.RemoveMultipleDataFromTempChart(d.item.idx);
      d.checked = false;
    });
  }

  datePickerChanged() {
    this.devices.filter(d => d.checked === true).forEach(d => {
      this.RemoveMultipleDataFromTempChart(d.item.idx);
      this.getAndAddDataForDevice(d.item);
    });
  }

  AddDeviceToGraph(event: Event, item: Device) {
    const cb: any = event.target;
    if (cb.checked === true) {
      this.getAndAddDataForDevice(item);
    } else {
      this.RemoveMultipleDataFromTempChart(item.idx);
    }
  }

  private getAndAddDataForDevice(item: Device): void {
    this.graphService.getDataCustom('temp', item.idx, this.from + 'T' + this.to, this.customgraphtype,
      this.graphTemp, this.graphChill, this.graphHum, this.graphBaro, this.graphDew, this.graphSet).subscribe(data => {
      this.AddMultipleDataToTempChart(data, this.customgraphtype, item.idx, item.Name);
    });
  }

  private AddMultipleDataToTempChart(data: GraphData, isday: string, deviceid: string, devicename: string) {
    const datatablete = [];
    const datatabletm = [];
    const datatablehu = [];
    const datatablech = [];
    const datatablecm = [];
    const datatabledp = [];
    const datatableba = [];
    const datatablese = [];
    const datatablesm = [];
    const datatablesx = [];

    data.result.forEach((item: GraphTempPoint) => {
      if (isday === '1') {
        if (typeof item.te !== 'undefined') {
          datatablete.push([Utils.GetUTCFromString(item.d), item.te]);
        }
        if (typeof item.hu !== 'undefined') {
          datatablehu.push([Utils.GetUTCFromString(item.d), item.hu]);
        }
        if (typeof item.ch !== 'undefined') {
          datatablech.push([Utils.GetUTCFromString(item.d), item.ch]);
        }
        if (typeof item.dp !== 'undefined') {
          datatabledp.push([Utils.GetUTCFromString(item.d), item.dp]);
        }
        if (typeof item.ba !== 'undefined') {
          datatableba.push([Utils.GetUTCFromString(item.d), item.ba]);
        }
        if (typeof item.se !== 'undefined') {
          datatablese.push([Utils.GetUTCFromString(item.d), item.se]);
        }
      } else {
        if (typeof item.te !== 'undefined') {
          datatablete.push([Utils.GetDateFromString(item.d), item.te]);
          datatabletm.push([Utils.GetDateFromString(item.d), item.tm]);
        }
        if (typeof item.hu !== 'undefined') {
          datatablehu.push([Utils.GetDateFromString(item.d), item.hu]);
        }
        if (typeof item.ch !== 'undefined') {
          datatablech.push([Utils.GetDateFromString(item.d), item.ch]);
          datatablecm.push([Utils.GetDateFromString(item.d), item.cm]);
        }
        if (typeof item.dp !== 'undefined') {
          datatabledp.push([Utils.GetDateFromString(item.d), item.dp]);
        }
        if (typeof item.ba !== 'undefined') {
          datatableba.push([Utils.GetDateFromString(item.d), item.ba]);
        }
        if (typeof item.se !== 'undefined') {
          datatablese.push([Utils.GetDateFromString(item.d), item.se]);
          datatablesm.push([Utils.GetDateFromString(item.d), item.sm]);
          datatablesx.push([Utils.GetDateFromString(item.d), item.sx]);
        }
      }
    });

    let series;

    if (datatablehu.length !== 0) {
      this.chart.addSeries(<Highcharts.SeriesLineOptions>
        {
          id: 'humidity' + deviceid,
          name: devicename + ':' + this.translationService.t('Humidity'),
          yAxis: 1
        }
      );
      series = this.chart.get('humidity' + deviceid);
      series.setData(datatablehu);
    }

    if (datatablech.length !== 0) {
      this.chart.addSeries(<Highcharts.SeriesLineOptions>
        {
          id: 'chill' + deviceid,
          name: devicename + ':' + this.translationService.t('Chill'),
          yAxis: 0
        }
      );
      series = this.chart.get('chill' + deviceid);
      series.setData(datatablech);

      if (isday === '0') {
        this.chart.addSeries(<Highcharts.SeriesLineOptions>
          {
            id: 'chillmin' + deviceid,
            name: devicename + ':' + this.translationService.t('Chill') + '_min',
            yAxis: 0
          }
        );
        series = this.chart.get('chillmin' + deviceid);
        series.setData(datatablecm);
      }
    }
    if (datatablete.length !== 0) {
      // Add Temperature series
      this.chart.addSeries(<Highcharts.SeriesLineOptions>
        {
          id: 'temperature' + deviceid,
          name: devicename + ':' + this.translationService.t('Temperature'),
          yAxis: 0
        }
      );
      series = this.chart.get('temperature' + deviceid);
      series.setData(datatablete);
      if (isday === '0') {
        this.chart.addSeries(<Highcharts.SeriesLineOptions>
          {
            id: 'temperaturemin' + deviceid,
            name: devicename + ':' + this.translationService.t('Temperature') + '_min',
            yAxis: 0
          }
        );
        series = this.chart.get('temperaturemin' + deviceid);
        series.setData(datatabletm);
      }
    }

    if (datatablese.length !== 0) {
      // Add Temperature series
      this.chart.addSeries(<Highcharts.SeriesLineOptions>
        {
          id: 'setpoint' + deviceid,
          name: devicename + ':' + this.translationService.t('SetPoint'),
          yAxis: 0
        }
      );
      series = this.chart.get('setpoint' + deviceid);
      series.setData(datatablese);
      if (isday === '0') {
        this.chart.addSeries(<Highcharts.SeriesLineOptions>
          {
            id: 'setpointmin' + deviceid,
            name: devicename + ':' + this.translationService.t('SetPoint') + '_min',
            yAxis: 0
          }
        );
        series = this.chart.get('setpointmin' + deviceid);
        series.setData(datatablesm);

        this.chart.addSeries(<Highcharts.SeriesLineOptions>
          {
            id: 'setpointmax' + deviceid,
            name: devicename + ':' + this.translationService.t('SetPoint') + '_max',
            yAxis: 0
          }
        );
        series = this.chart.get('setpointmax' + deviceid);
        series.setData(datatablesx);
      }
    }

    if (datatabledp.length !== 0) {
      this.chart.addSeries(<Highcharts.SeriesLineOptions>
        {
          id: 'dewpoint' + deviceid,
          name: devicename + ':' + this.translationService.t('Dew Point'),
          yAxis: 0
        }
      );
      series = this.chart.get('dewpoint' + deviceid);
      series.setData(datatabledp);
    }

    if (datatableba.length !== 0) {
      this.chart.addSeries(<Highcharts.SeriesLineOptions>
        {
          id: 'baro' + deviceid,
          name: devicename + ':' + this.translationService.t('Barometer'),
          yAxis: 2
        }
      );
      series = this.chart.get('baro' + deviceid);
      series.setData(datatableba);
    }
  }

  private RemoveMultipleDataFromTempChart(deviceid: string) {
    const hum = <Highcharts.Series>this.chart.get('humidity' + deviceid);
    if (hum != null) {
      hum.remove();
    }
    const chill = <Highcharts.Series>this.chart.get('chill' + deviceid);
    if (chill != null) {
      chill.remove();
    }
    const chillmin = <Highcharts.Series>this.chart.get('chillmin' + deviceid);
    if (chillmin != null) {
      chillmin.remove();
    }
    const temperature = <Highcharts.Series>this.chart.get('temperature' + deviceid);
    if (temperature != null) {
      temperature.remove();
    }
    const temperaturemin = <Highcharts.Series>this.chart.get('temperaturemin' + deviceid);
    if (temperaturemin != null) {
      temperaturemin.remove();
    }
    const dew = <Highcharts.Series>this.chart.get('dewpoint' + deviceid);
    if (dew != null) {
      dew.remove();
    }
    const baro = <Highcharts.Series>this.chart.get('baro' + deviceid);
    if (baro != null) {
      baro.remove();
    }
    const setpoint = <Highcharts.Series>this.chart.get('setpoint' + deviceid);
    if (setpoint != null) {
      setpoint.remove();
    }
    const setpointmin = <Highcharts.Series>this.chart.get('setpointmin' + deviceid);
    if (setpointmin != null) {
      setpointmin.remove();
    }
    const setpointmax = <Highcharts.Series>this.chart.get('setpointmax' + deviceid);
    if (setpointmax != null) {
      setpointmax.remove();
    }
  }

}

class DeviceCheck {
  item: Device;
  checked: boolean;
}
