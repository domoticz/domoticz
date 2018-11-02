import {AfterViewInit, Component, ElementRef, EventEmitter, Input, OnInit, Output, ViewChild} from '@angular/core';
import {LightSwitchesService} from 'src/app/_shared/_services/light-switches.service';
import {mergeMap} from 'rxjs/operators';
import {DeviceService} from '../../_services/device.service';
import {Device} from '../../_models/device';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-selectorlevels',
  templateUrl: './selectorlevels.component.html',
  styleUrls: ['./selectorlevels.component.css']
})
export class SelectorlevelsComponent implements OnInit, AfterViewInit {

  @Input() item: Device;
  @Output() itemChange: EventEmitter<Device> = new EventEmitter<Device>();

  @Input() levelNames: Array<string>;

  @ViewChild('select', {static: false}) selectRef: ElementRef;

  constructor(
    private lightSwitchesService: LightSwitchesService,
    private deviceService: DeviceService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {

    const _this = this;

    // Create Selector selectmenu
    $(this.selectRef.nativeElement).selectmenu({
      // Config
      width: '75%',
      value: 0,

      // Selector selectmenu events
      create: function (event, ui) {
        const select$ = $(this);
        // const idx = _this.item.idx;
        // const isprotected = _this.item.Protected;
        const disabled = select$.data('disabled');
        const level = _this.item.LevelInt;
        // const levelname = Utils.GetLightStatusText(_this.item);
        // select$.selectmenu('option', 'idx', idx);
        // select$.selectmenu('option', 'isprotected', isprotected);
        select$.selectmenu('option', 'disabled', disabled === true);
        select$.selectmenu('menuWidget').addClass('selectorlevels-menu');
        select$.val(level);
      },

      change: function (event, ui) { // When the user selects an option
        const select$ = $(this);
        const idx = _this.item.idx;
        const level = select$.selectmenu().val();
        const levelname = select$.find('option[value="' + level + '"]').text();
        const isprotected = _this.item.Protected;
        // Send command
        _this.lightSwitchesService.SwitchSelectorLevel(idx, levelname, level, isprotected)
          .pipe(
            mergeMap(() => _this.deviceService.getDeviceInfo(_this.item.idx))
          )
          .subscribe((updatedItem) => {
            _this.item = updatedItem;
            _this.itemChange.emit(updatedItem);
          });
        // Synchronize buttons and select attributes
        select$.data('level', level);
        select$.data('levelname', levelname);
      }
    }).selectmenu('refresh');
  }

  public Resize() {
    $(this.selectRef.nativeElement).selectmenu('option', 'width', '75%');
  }

}
