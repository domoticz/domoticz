import { Component, OnInit, Input, OnChanges, SimpleChanges } from '@angular/core';
import { ControlValueAccessor, NG_VALUE_ACCESSOR } from '@angular/forms';
import { DeviceService } from '../../_shared/_services/device.service';
import { CustomLightIcon } from 'src/app/_shared/_models/custom-light-icons';
import { Observable } from 'rxjs';
import { defaultSwitchIcons } from 'src/app/_shared/_constants/default-switch-icons';

// FIXME manage with NPM+TypeScript
declare var $: any;

@Component({
  selector: 'dz-device-icon-select',
  templateUrl: './device-icon-select.component.html',
  styleUrls: ['./device-icon-select.component.css'],
  providers: [{
    provide: NG_VALUE_ACCESSOR,
    useExisting: DeviceIconSelectComponent,
    multi: true
  }]
})
export class DeviceIconSelectComponent implements OnInit, OnChanges, ControlValueAccessor {

  @Input() switchType: number;

  private switch_icons: Array<CustomLightIcon> = [];

  private lighticonsObs: Observable<CustomLightIcon[]>;

  private propagateChange = (_: any) => { };

  constructor(
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
    const _this = this;
    this.lighticonsObs = this.deviceService.getCustomLightIconsForDeviceIconSelect();

    this.lighticonsObs.subscribe(switch_icons => {

      this.switch_icons = switch_icons;

      this.switch_icons.unshift({
        text: 'Default',
        value: 0,
        selected: false,
        description: 'Default icon'
      });

      this.updateSelector(false);

    });
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.switchType && this.switch_icons.length > 0) {
      this.updateSelector(true);
    }
  }

  writeValue(value: any) {
    if (value !== undefined) {
      // Wait for icons to be retrieved...
      this.lighticonsObs.subscribe(switch_icons => {
        this.switch_icons.forEach((item, index) => {
          if (item.value === value) {
            $('#icon-select').ddslick('select', { index: index });
          }
        });
      });
    }
  }

  registerOnChange(fn) {
    this.propagateChange = fn;
  }

  registerOnTouched() { }

  private updateSelector(bFromUser: boolean) {
    const _this = this;

    this.switch_icons[0].imageSrc = defaultSwitchIcons[this.switchType]
      ? 'images/' + defaultSwitchIcons[this.switchType][0]
      : 'images/Generic48_On.png';

    // FIXME remove ddslick thing
    $('#icon-select').ddslick('destroy');
    $('#icon-select').ddslick({
      data: this.switch_icons,
      width: 260,
      height: 390,
      selectText: 'Select Switch Icon',
      imagePosition: 'left',
      onSelected: (data) => {
        _this.propagateChange(data.selectedData.value);
      }
    });

    if (bFromUser) {
      $('#icon-select').ddslick('select', { index: 0 });
    }
  }

}
