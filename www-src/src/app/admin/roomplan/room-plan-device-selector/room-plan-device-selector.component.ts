import {Component, EventEmitter, Input, OnChanges, OnInit, Output, SimpleChanges} from '@angular/core';
import {PlanDeviceItem} from '../room-plan-device-selector-modal/room-plan-device-selector-modal.component';

@Component({
  selector: 'dz-room-plan-device-selector',
  templateUrl: './room-plan-device-selector.component.html',
  styleUrls: ['./room-plan-device-selector.component.css']
})
export class RoomPlanDeviceSelectorComponent implements OnInit, OnChanges {

  @Input() devices: Array<PlanDeviceItem> = [];

  @Output() select: EventEmitter<PlanDeviceItem | null> = new EventEmitter<PlanDeviceItem | null>();

  hardwareFilter = '';
  hardwareItems: Array<string> = [];
  filteredDevices: Array<PlanDeviceItem> = [];
  selectedDevice: PlanDeviceItem | null;

  constructor() {
  }

  ngOnInit() {
    this.updateDevices();
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes.devices) {
      this.updateDevices();
    }
  }

  private updateDevices() {
    this.hardwareItems = Array.from(new Set(this.devices.map(device => device.Hardware))).sort();
    this.filterByHardware(this.hardwareFilter);
  }

  private filterByHardware(hardware: string) {
    this.hardwareFilter = this.hardwareFilter !== hardware ? hardware : '';

    this.filteredDevices = this.devices.filter(dev => {
      return dev.Hardware.includes(this.hardwareFilter);
    });
  }

  selectDevice(device: PlanDeviceItem | null) {
    this.selectedDevice = device;
    this.select.emit(this.selectedDevice);
  }

}
