import {
    AfterViewInit,
    Component,
    ElementRef,
    Inject,
    Input,
    OnChanges,
    OnInit,
    SimpleChanges,
    ViewChild
} from '@angular/core';
import {
    CounterReportData,
    DayReportData,
    DeviceCounterReportDataService,
    MonthReportData
} from '../device-counter-report-data.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import * as dateFormat from 'dateformat';
import {ConfigService} from '../../_shared/_services/config.service';
import * as Highcharts from 'highcharts';
import {DatatableHelper} from '../../_shared/_utils/datatable-helper';
import {Router} from '@angular/router';
import {DeviceUtils} from '../../_shared/_utils/device-utils';
import {Device} from '../../_shared/_models/device';
import {ChartService} from '../../_shared/_services/chart.service';

// FIXME use proper ts def
declare var $: any;

@Component({
    selector: 'dz-device-counter-report',
    templateUrl: './device-counter-report.component.html',
    styleUrls: ['./device-counter-report.component.css']
})
export class DeviceCounterReportComponent implements OnInit, AfterViewInit, OnChanges {

    @Input() device: Device;
    @Input() selectedYear: number;
    @Input() selectedMonth?: number;
    @Input() isOnlyUsage: boolean;

    @ViewChild('reporttable', {static: false}) tableRef: ElementRef;
    @ViewChild('usagegraph', {static: false}) usagegraph: ElementRef;

    unit: string;
    isMonthView: boolean;
    noDataAvailable = false;
    data: CounterReportData;

    constructor(
        private deviceCounterReportDataService: DeviceCounterReportDataService,
        @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
        private configService: ConfigService,
        private router: Router
    ) {
    }

    ngOnInit() {
        this.unit = DeviceUtils.getUnit(this.device);
        this.isMonthView = this.selectedMonth > 0;
    }

    ngAfterViewInit() {
        this.getData();
    }

    ngOnChanges(changes: SimpleChanges) {
        if ((changes.selectedYear && !changes.selectedYear.isFirstChange()) ||
            (changes.selectedMonth && !changes.selectedMonth.isFirstChange())) {
            this.getData();
        }
    }

    private getData() {
        this.deviceCounterReportDataService.fetch(this.device, this.selectedYear, this.selectedMonth).subscribe((data) => {
            if (!data) {
                this.noDataAvailable = true;
                return;
            } else {
                this.noDataAvailable = false;
            }

            this.data = data;

            this.showTable(data);
            this.showUsageChart(data);
        });
    }

    private showTable(data: CounterReportData) {

        // Remove previous table (in case of input changes)
        if ($.fn.DataTable.isDataTable(this.tableRef.nativeElement)) {
            $(this.tableRef.nativeElement).DataTable().clear();
            $(this.tableRef.nativeElement).DataTable().destroy();
        }

        const table = $(this.tableRef.nativeElement);
        const columns = [];

        const counterRenderer = (d: number) => {
            return d.toFixed(3);
        };

        const costRenderer = (d: number) => {
            return d.toFixed(2);
        };

        if (this.isMonthView) {
            columns.push({
                title: this.translationService.t('Day'),
                data: 'date',
                render: (d) => {
                    return this.translationService.t(dateFormat(d, 'd'));
                }
            });

            columns.push({
                title: '',
                data: 'date',
                render: (d) => {
                    return this.translationService.t(dateFormat(d, 'dddd'));
                }
            });
        } else {
            columns.push({
                title: this.translationService.t('Month'),
                data: 'date',
                render: (d) => {
                    const date = new Date(d);
                    const link = '<a class="ng-link" href="/Devices/' + this.device.idx + '/Report/' + this.selectedYear + '/' +
                        (date.getMonth() + 1) + '"><img src="images/next.png" /></a>';
                    return dateFormat(d, 'mm. mmmm') + ' ' + link;
                }
            });
        }

        if (this.isMonthView && !this.isOnlyUsage) {
            columns.push({title: this.translationService.t('Counter'), data: 'counter', render: counterRenderer});
        }

        columns.push({
            title: this.translationService.t(this.device.SwitchTypeVal === 4 ? 'Generated' : 'Usage'),
            data: 'usage',
            render: counterRenderer
        });

        if (!['Counter Incremental'].includes(this.device.SubType) && (this.device.SwitchTypeVal !== 3)) {
            columns.push({
                title: this.translationService.t(this.device.SwitchTypeVal === 4 ? 'Earnings' : 'Costs'),
                data: 'cost',
                render: costRenderer
            });
        }

        columns.push({
            title: '<>',
            orderable: false,
            data: 'trend',
            render: (d) => {
                return '<img src="images/' + d + '.png">';
            }
        });

        table.dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
            sDom: '<"H"rC>t<"F">',
            columns: columns,
            pageLength: 50,
            order: [[0, 'asc']]
        }));

        table.dataTable().api().rows
            .add(data.items)
            .draw();

        // Fix links so that it uses Angular router
        DatatableHelper.fixAngularLinks('.ng-link', this.router);
    }

    private showUsageChart(data: CounterReportData) {
        const series = [];
        let valueQuantity = 'Count';
        if (typeof this.device.ValueQuantity !== 'undefined') {
            valueQuantity = this.device.ValueQuantity;
        }

        const chartName = this.device.SwitchTypeVal === 4 ? 'Generated' : 'Usage';
        const yAxisName = ['Energy', 'Gas', 'Water', valueQuantity, 'Energy'][this.device.SwitchTypeVal];

        series.push({
            name: this.translationService.t(chartName),
            color: 'rgba(3,190,252,0.8)',
            stack: 'susage',
            yAxis: 0,
            data: (data.items as Array<DayReportData | MonthReportData>).map((item) => {
                return {
                    x: +(new Date(item.date)),
                    y: parseFloat(item.usage.toFixed(3))
                };
            })
        });

        const chartOptions: Highcharts.Options = {
            chart: {
                type: 'column',
            },
            title: {
                text: ''
            },
            xAxis: {
                type: 'datetime'
            },
            yAxis: {
                title: {
                    text: this.translationService.t(yAxisName) + ' (' + this.unit + ')'
                },
                maxPadding: 0.2,
                min: 0
            },
            tooltip: {
                valueSuffix: ' ' + this.unit,
                valueDecimals: 3
            },
            plotOptions: {
                column: {
                    minPointLength: 4,
                    pointPadding: 0.1,
                    groupPadding: 0,
                    dataLabels: {
                        enabled: true,
                        color: 'white'
                    }
                }
            },
            legend: {
                enabled: true
            },
            series: series
        };

        ChartService.setDeviceIdx(this.device.idx);
        Highcharts.chart(this.usagegraph.nativeElement, chartOptions);
    }

}
