import { Component, OnInit, Input, Output, EventEmitter, OnChanges } from '@angular/core';

@Component({
  selector: 'dz-device-filter-by-usage',
  templateUrl: './device-filter-by-usage.component.html',
  styleUrls: ['./device-filter-by-usage.component.css']
})
export class DeviceFilterByUsageComponent implements OnInit, OnChanges {

  @Input() filter: any;
  @Output() filterChange: EventEmitter<any> = new EventEmitter<any>();

  value?: number;

  constructor() { }

  ngOnInit() {
  }

  ngOnChanges() {
    const formatter: (_: any) => number | undefined = (value) => {
      const filterValue = value && value.Used;

      return (Array.isArray(filterValue) && filterValue.length === 1) ? filterValue[0] : undefined;
    };

    this.value = formatter(this.filter);
  }

  setFilter(value?: number) {
    const filterValue = value !== undefined ? [value] : [0, 1];

    this.value = value;

    const parser: (_: Array<number>) => any = (v: Array<number>) => {
      return Object.assign({}, this.value, {
        Used: v && v.length === 1 ? v : undefined
      });
    };

    this.filterChange.emit(parser(filterValue));
  }

}
