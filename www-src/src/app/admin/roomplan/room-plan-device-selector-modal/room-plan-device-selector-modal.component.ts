import {Component, ElementRef, EventEmitter, OnInit, Output, ViewChild} from '@angular/core';
import {RoomService} from '../../../_shared/_services/room.service';
import {UnusedPlanDevice} from '../../../_shared/_models/plan';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-room-plan-device-selector-modal',
  templateUrl: './room-plan-device-selector-modal.component.html',
  styleUrls: ['./room-plan-device-selector-modal.component.css']
})
export class RoomPlanDeviceSelectorModalComponent implements OnInit {

  @ViewChild('modal', {static: false}) modalRef: ElementRef;

  devices: Array<PlanDeviceItem> = [];
  selectedDevice: PlanDeviceItem | null;

  selected: EventEmitter<PlanDeviceItem> = new EventEmitter<PlanDeviceItem>();

  constructor(private roomService: RoomService) {
  }

  ngOnInit() {
    this.refreshPlanAvailableDevices();
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  private refreshPlanAvailableDevices() {
    this.roomService.getPlanAvailableDevices().subscribe(devices => {
      const regex = new RegExp('^\\[(.*)\] (.*)$');

      this.devices = devices.map(device => {
        const result = regex.exec(device.Name);

        return Object.assign({}, device, {
          Hardware: result[1],
          Name: result[2],
        });
      });
    });
  }

  selectDevice(device: PlanDeviceItem | null) {
    this.selectedDevice = device;
  }

  close() {
    this.selected.emit(this.selectedDevice);
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}

export interface PlanDeviceItem extends UnusedPlanDevice {
  Hardware: string;
}
