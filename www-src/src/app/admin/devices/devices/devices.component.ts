import {Component, OnInit} from '@angular/core';
import {DeviceService} from '../../../_shared/_services/device.service';
import {Device} from '../../../_shared/_models/device';
import {LivesocketService} from '../../../_shared/_services/livesocket.service';
import {DomoticzDevicesService} from '../../../_shared/_services/domoticz-devices.service';
import {DeviceUtils} from "../../../_shared/_utils/device-utils";

@Component({
  selector: 'dz-devices',
  templateUrl: './devices.component.html',
  styleUrls: ['./devices.component.css']
})
export class DevicesComponent implements OnInit {

  devices: Array<Device>;

  filter: any;
  filteredDevices: Device[];

  isListExpanded = false;

  constructor(
    private deviceService: DeviceService,
    private domoticzDevicesService: DomoticzDevicesService,
    private livesocketService: LivesocketService
  ) {
  }

  ngOnInit() {
    this.isListExpanded = false;
    this.filter = {};
    this.refreshDevices();

    this.livesocketService.device_update.subscribe(deviceData => this.updateItem(deviceData));
    this.livesocketService.scene_update.subscribe(deviceData => this.updateItem(deviceData));
  }

  private updateItem(deviceData) {
    const device = this.devices.find((dev) => {
      return dev.idx === deviceData.idx;
    });

    if (device) {
      Object.assign(device, deviceData);
    } else {
      this.devices.push(deviceData);
    }
  }

  refreshDevices() {
    return this.deviceService.getAllDevices().subscribe(response => {
      if (response.result !== undefined) {
        this.devices = response.result.map((item) => {
          const isScene = DeviceUtils.isScene(item);

          if (isScene) {
            item.HardwareName = 'Domoticz';
            item.ID = '-';
            item.Unit = '-';
            item.SubType = '-';
            item.SignalLevel = '-';
            item.BatteryLevel = 255;
          }

          // return new DomoticzDevice(this.domoticzDevicesService, item);
          return item;
        });
      } else {
        this.devices = [];
      }
      this.applyFilter();
    });
  }

  applyFilter() {
    this.filteredDevices = (this.devices || []).filter((device) => {
      return Object.keys(this.filter)
        .filter((fieldName) => {
          return this.filter[fieldName] !== undefined;
        })
        .every((fieldName) => {
          return Array.isArray(device[fieldName])
            ? device[fieldName].some((value) => {
              return this.filter[fieldName].includes(value);
            })
            : this.filter[fieldName].includes(device[fieldName]);
        });
    });
  }

}
