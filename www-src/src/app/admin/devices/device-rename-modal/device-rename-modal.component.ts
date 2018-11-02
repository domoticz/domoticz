import { Component, OnInit, ViewChild, ElementRef, Output, EventEmitter } from '@angular/core';
import { SceneService } from '../../../_shared/_services/scene.service';
import { DeviceService } from '../../../_shared/_services/device.service';
import {Device} from "../../../_shared/_models/device";

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-rename-modal',
  templateUrl: './device-rename-modal.component.html',
  styleUrls: ['./device-rename-modal.component.css']
})
export class DeviceRenameModalComponent implements OnInit {

  device: Device;
  isSaving = false;

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  @Output() renamed: EventEmitter<void> = new EventEmitter<void>();

  constructor(
    private sceneService: SceneService,
    private deviceService: DeviceService
  ) { }

  ngOnInit() {
  }

  open(device: Device) {
    this.device = device;
    $(this.modalRef.nativeElement).modal('show');
  }

  renameDevice() {
    this.isSaving = true;

    const savingPromise = ['Scene', 'Group'].includes(this.device.Type)
      ? this.sceneService.renameScene(this.device.idx, this.device.Name)
      : this.deviceService.renameDevice(this.device.idx, this.device.Name);

    savingPromise.subscribe(() => {
      this.close();
    });
  }

  close() {
    this.renamed.emit();
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}
