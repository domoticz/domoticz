import { Component, OnInit, Output, EventEmitter, ViewChild, ElementRef } from '@angular/core';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-edit-selector-action-modal',
  templateUrl: './device-edit-selector-action-modal.component.html',
  styleUrls: ['./device-edit-selector-action-modal.component.css']
})
export class DeviceEditSelectorActionModalComponent implements OnInit {

  level: number;
  action: string;

  @Output() rename: EventEmitter<[number, string]> = new EventEmitter<[number, string]>();

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  constructor() { }

  ngOnInit() {
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  close() {
    this.rename.emit([this.level, this.action]);
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}
