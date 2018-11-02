import {DeviceService} from '../_shared/_services/device.service';
import {GraphService} from '../_shared/_services/graph.service';
import {forkJoin, Observable} from 'rxjs';
import {CostsResponse} from '../_shared/_models/costs';
import {GraphTempPoint} from '../_shared/_models/graphdata';
import {ReportHelpers} from '../_shared/_utils/report-helpers';
import {map} from 'rxjs/operators';
import {Device} from '../_shared/_models/device';
import {Injectable} from '@angular/core';

@Injectable()
export class DeviceCounterReportDataService {

  constructor(
    private deviceService: DeviceService,
    private graphService: GraphService
  ) {
  }

  public fetch(device: Device, year: number, month?: number): Observable<CounterReportData> {
    const _costs = this.deviceService.getCosts(device.idx);

    const _stats = this.graphService.getDataWithActYearAndMonth('year', device.idx, 'counter', year, month);

    return forkJoin(_costs, _stats).pipe(
      map(responses => {
        const cost = this.getCost(device, responses[0]);
        const stats = responses[1];

        if (!stats.result || !stats.result.length) {
          return null;
        }

        const data = this.getGroupedData(stats.result, cost);
        const source: MonthReportData | YearReportData = month
          ? data.years[year].months.find((item) => {
            return (new Date(item.date)).getMonth() + 1 === month;
          })
          : data.years[year];

        if (!source) {
          return null;
        }

        return {
          cost: source.cost,
          usage: source.usage,
          counter: month ? source.counter : Number(stats.counter),
          items: month ? (source as MonthReportData).days : (source as YearReportData).months
        };
      })
    );
  }

  private getCost(device: Device, costs: CostsResponse): number {
    const switchTypeVal = device.SwitchTypeVal;

    if (switchTypeVal === 0 || switchTypeVal === 4) {
      return costs.CostEnergy / 10000;
    } else if (switchTypeVal === 1) {
      return costs.CostGas / 10000;
    } else if (switchTypeVal === 2) {
      return costs.CostWater / 10000;
    }
  }

  private getGroupedData(data: Array<GraphTempPoint>, cost: number): GroupReportData {
    const result = {
      years: {}
    };

    data.forEach((item) => {
      const month = parseInt(item.d.substring(5, 7), 10);
      const year = parseInt(item.d.substring(0, 4), 10);
      const day = parseInt(item.d.substring(8, 10), 10);

      if (!result.years[year]) {
        result.years[year] = {
          months: {}
        };
      }

      if (!result.years[year].months[month]) {
        result.years[year].months[month] = {
          date: Date.UTC(year, month - 1, 1),
          days: {}
        };
      }

      result.years[year].months[month].days[day] = {
        date: item.d,
        usage: Number(item.v),
        counter: Number(item.c),
        cost: Number(item.v) * cost
      };
    });

    Object.keys(result.years).forEach((year) => {
      const yearsData = result.years[year];
      yearsData.months = Object.values(yearsData.months);

      yearsData.months.sort((a, b) => {
        return a.date < b.date ? -1 : 1;
      });

      yearsData.months.forEach((month) => {
        month.days = Object.values(month.days);

        month.days.sort((a, b) => {
          return a.date < b.date ? -1 : 1;
        });

        month.days = ReportHelpers.addTrendData(month.days, 'usage');
        Object.assign(month, this.getGroupStats(month.days));
      });

      yearsData.months = ReportHelpers.addTrendData(yearsData.months, 'usage');
      Object.assign(yearsData, this.getGroupStats(yearsData.months));
    });

    return result;
  }

  private getGroupStats(values: Array<ReportData>): ReportData {
    const keys = ['usage', 'cost'];

    return values.reduce((acc: any, item: ReportData) => {
      keys.forEach((key: string) => {
        acc[key] = item[key] !== null ? (acc[key] || 0) + item[key] : null;
      });

      acc.counter = Math.max(acc.counter || 0, item.counter);
      return acc;
    }, {});
  }

}

export interface CounterReportData {
  cost: number;
  usage: number;
  counter: number;
  items: Array<DayReportData> | Array<MonthReportData>;
}

export interface ReportData {
  cost: number;
  usage: number;
  counter: number;
}

export interface GroupReportData {
  years: { [y: number]: YearReportData };
}

export interface YearReportData {
  months: Array<MonthReportData>;
  cost: number;
  usage: number;
  counter: number;
}

export interface MonthReportData extends ReportData {
  days: Array<DayReportData>;
  date: number;
  trend: string;
}

export interface DayReportData extends ReportData {
  date: string;
  trend: string;
}
