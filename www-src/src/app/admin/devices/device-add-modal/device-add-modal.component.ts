import {Component, ElementRef, EventEmitter, OnInit, Output, ViewChild} from '@angular/core';
import {DeviceService} from '../../../_shared/_services/device.service';
import {LightSwitchesService} from '../../../_shared/_services/light-switches.service';
import {LightSwitch} from '../../../_shared/_models/lightswitches';
import {Device} from "../../../_shared/_models/device";

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-add-modal',
  templateUrl: './device-add-modal.component.html',
  styleUrls: ['./device-add-modal.component.css']
})
export class DeviceAddModalComponent implements OnInit {

  device: Device;
  isLightDevice = false;
  isMainDevice = true;
  mainDevice: string;
  mainDevices: Array<LightSwitch> = [];
  isSaving = false;

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  @Output() added: EventEmitter<void> = new EventEmitter<void>();

  constructor(
    private deviceService: DeviceService,
    private lightSwitchesService: LightSwitchesService
  ) { }

  ngOnInit() {
  }

  open(device: Device) {
    this.init(device);
    $(this.modalRef.nativeElement).modal('show');
  }

  init(device: Device) {
    this.device = device;
    this.isLightDevice = this.device.Type.indexOf('Light') === 0 || this.device.Type.indexOf('Security') === 0;
    this.isMainDevice = true;

    if (this.isLightDevice) {
      this.lightSwitchesService.getlightswitches().subscribe(response => {
        this.mainDevices = response.result || [];
      });
    }
  }

  addDevice() {
    this.isSaving = true;
    const mainDevice = this.isMainDevice ? undefined : this.mainDevice;

    this.deviceService.includeDevice(this.device.idx, this.device.Name, mainDevice).subscribe(() => {
      this.close();
    });
  }

  close() {
    this.added.emit();
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}
