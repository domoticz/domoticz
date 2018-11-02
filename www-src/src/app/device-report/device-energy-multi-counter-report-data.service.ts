import {DeviceService} from '../_shared/_services/device.service';
import {GraphService} from '../_shared/_services/graph.service';
import {forkJoin, Observable} from 'rxjs';
import {map} from 'rxjs/operators';
import {CostsResponse} from '../_shared/_models/costs';
import {GraphData, GraphTempPoint} from '../_shared/_models/graphdata';
import {ReportHelpers} from '../_shared/_utils/report-helpers';
import {Injectable} from '@angular/core';

@Injectable()
export class DeviceEnergyMultiCounterReportDataService {

  constructor(
    private deviceService: DeviceService,
    private graphService: GraphService
  ) { }

  public fetch(deviceIdx: string, year: number, month?: number): Observable<EnergyMultiCounterReportData> {
    const _costs = this.deviceService.getCosts(deviceIdx);

    const _stats = this.graphService.getDataWithActYearAndMonth('year', deviceIdx, 'counter', year, month);

    return forkJoin(_costs, _stats).pipe(
      map(responses => {
        const costs: CostsResponse = responses[0];
        const stats: GraphData = responses[1];

        if (!stats.result || !stats.result.length) {
          return null;
        }

        const includeReturn = typeof stats.delivered !== 'undefined';
        const data = this.getGroupedData(stats.result, costs, includeReturn);
        const source = month
          ? data.years[year].months.find((item) => {
            return (new Date(item.date)).getMonth() + 1 === month;
          })
          : data.years[year];

        if (!source) {
          return null;
        }

        return Object.assign({}, source, {
          items: month ? source.days : source.months
        });
      })
    );
  }

  private getGroupedData(data: Array<GraphTempPoint>, costs: CostsResponse, includeReturn: boolean): any {
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

      const dayRecord: any = {
        date: item.d,
        usage1: {
          usage: Number(item.v),
          cost: Number(item.v) * costs.CostEnergy / 10000,
          counter: Number(item.c1) || 0
        },
        usage2: {
          usage: Number(item.v2),
          cost: Number(item.v2) * costs.CostEnergyT2 / 10000,
          counter: Number(item.c3) || 0
        },
      };

      if (includeReturn) {
        dayRecord.return1 = {
          usage: Number(item.r1),
          cost: Number(item.r1) * costs.CostEnergyR1 / 10000,
          counter: Number(item.c2) || 0
        };

        dayRecord.return2 = {
          usage: Number(item.r2),
          cost: Number(item.r2) * costs.CostEnergyR2 / 10000,
          counter: Number(item.c4) || 0
        };

        dayRecord.totalReturn = dayRecord.return1.usage + dayRecord.return2.usage;
      }

      dayRecord.totalUsage = dayRecord.usage1.usage + dayRecord.usage2.usage;
      dayRecord.usage = dayRecord.totalUsage - (includeReturn ? dayRecord.totalReturn : 0);
      dayRecord.cost = dayRecord.usage1.cost + dayRecord.usage2.cost -
        (includeReturn ? (dayRecord.return1.cost + dayRecord.return2.cost) : 0);
      result.years[year].months[month].days[day] = dayRecord;
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

  private getGroupStats(values): any {
    const keys = [
      'usage1', 'usage2', 'return1', 'return2',
    ];

    return values.reduce((acc, item) => {
      keys.forEach((key) => {
        if (!item[key]) {
          return;
        }

        if (!acc[key]) {
          acc[key] = {};
        }

        acc[key].usage = (acc[key].usage || 0) + item[key].usage;
        acc[key].cost = (acc[key].cost || 0) + item[key].cost;
        acc[key].counter = Math.max(acc[key].counter || 0, item[key].counter);
      });

      acc.totalUsage = (acc.totalUsage || 0) + item.totalUsage;
      acc.totalReturn = (acc.totalReturn || 0) + item.totalReturn; acc.usage = (acc.usage || 0) + item.usage;
      acc.cost = (acc.cost || 0) + item.cost;
      return acc;
    }, {});
  }

}

export interface EnergyMultiCounterReportData {
  usage1: UsageCostCounter;
  usage2: UsageCostCounter;
  return1?: UsageCostCounter;
  return2?: UsageCostCounter;
  items: Array<any>;
}

export interface UsageCostCounter {
  usage: number;
  cost: number;
  counter: number;
}

