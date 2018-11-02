import {Component, EventEmitter, Inject, Input, OnChanges, OnInit, Output, SimpleChanges} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {RoomService} from '../../../_shared/_services/room.service';
import {Observable} from 'rxjs';
import {Plan} from 'src/app/_shared/_models/plan';
import {Device} from '../../../_shared/_models/device';

@Component({
  selector: 'dz-devices-filters',
  templateUrl: './devices-filters.component.html',
  styleUrls: ['./devices-filters.component.css']
})
export class DevicesFiltersComponent implements OnInit, OnChanges {

  @Input() devices: Array<Device>;

  @Input() filter: any;
  @Output() filterChange: EventEmitter<any> = new EventEmitter<any>();

  filters: Array<Filter>;
  filterValue: any = {};
  filterValues: any[] = [];

  filterAdditionalDataPromise: Observable<Array<Plan>>;
  plans: Array<any>;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private roomService: RoomService
  ) {
  }

  ngOnInit() {
    this.filters = [
      {
        field: 'Used',
        name: this.translationService.t('Used'),
        display: (value: number) => {
          return value === 0 ? 'No' : 'Yes';
        },
        parse: (value: string) => {
          return parseInt(value, 10);
        }
      },
      {
        field: 'HardwareName',
        name: this.translationService.t('Hardware')
      },
      {
        field: 'Type',
        name: this.translationService.t('Type')
      },
      {
        field: 'PlanIDs',
        name: this.translationService.t('Room'),
        display: (value: number) => {
          const roomPlan = this.plans.find((item) => {
            return Number(item.idx) === value;
          });

          if (roomPlan) {
            return roomPlan['Name'];
          } else {
            return this.translationService.t('- N/A -');
          }
        },
        parse: (value: string) => {
          return parseInt(value, 10);
        }
      }
    ];

    this.filters = this.filters.map((filter: Filter) => {
      return Object.assign({collapsed: false}, filter);
    });

    this.initFilterValue();
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.filterAdditionalDataPromise) {
      this.filterAdditionalDataPromise = this.loadRooms();
    }

    if (changes.devices && changes.devices.currentValue) {
      this.filterAdditionalDataPromise.subscribe(() => {
        this.initFilters(this.devices);
      });
    }

    this.filterValue = Object.keys(this.filter)
      .filter((fieldName) => {
        return this.filter[fieldName] !== undefined;
      })
      .reduce((acc, fieldName) => {
        const fieldFilterValue = this.filter[fieldName].reduce((acc2, fieldValue) => {
          acc2[fieldValue] = true;
          return acc2;
        }, {});

        acc[fieldName] = fieldFilterValue;
        return acc;
      }, this.filterValue);
  }

  private loadRooms(): Observable<Array<Plan>> {
    const promise = this.roomService.getPlansWithoutHidden();
    promise.subscribe(plans => {
      this.plans = plans;
    });
    return promise;
  }

  initFilters(devices: Array<Device>) {
    this.filterValues = (devices || [])
      .reduce((acc, device: Device) => {
        this.filters.forEach((item: Filter, index: number) => {
          if (!acc[index]) {
            acc[index] = [];
          }

          if (device[item.field] === undefined) {
            return;
          }

          const values = Array.isArray(device[item.field]) ? device[item.field] : [device[item.field]];

          values.forEach((value) => {
            if (!acc[index].includes(value)) {
              acc[index].push(value);
            }
          });
        });

        return acc;
      }, [])
      .map((values, filterIndex) => {
        const idFn: (number) => string = (v) => {
          return v;
        };
        const displayFn: (number) => string = this.filters[filterIndex].display || idFn;

        values.sort((value1, value2) => {
          return displayFn(value1) > displayFn(value2) ? 1 : -1;
        });

        return values;
      });
  }

  updateFilterValue() {
    const value = Object.keys(this.filterValue).reduce((acc, fieldName) => {
      const filter = this.filters.find((item) => {
        return item.field === fieldName;
      });

      const filterFieldValue = Object.keys(this.filterValue[fieldName])
        .filter((item) => {
          return this.filterValue[fieldName][item] === true;
        })
        .map((v) => {
          return filter.parse ? filter.parse(v) : v;
        });

      if (filterFieldValue.length > 0) {
        acc[fieldName] = filterFieldValue;
      }

      return acc;
    }, {});

    this.filterChange.emit(value);
  }

  private initFilterValue() {
    this.filters.forEach((f, index) => {
      this.filterValue[f.field] = {};
      this.filterValues.forEach((item) => {
        this.filterValue[f.field][item] = false;
      });
    });
  }

}

class Filter {
  field: string;
  name: string;
  display?: (value: number) => string;
  parse?: (value: string) => number;
  collapsed?: boolean;
}
