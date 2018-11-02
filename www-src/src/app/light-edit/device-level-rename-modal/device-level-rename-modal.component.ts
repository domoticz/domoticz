import { Component, OnInit, ViewChild, ElementRef, EventEmitter, Output } from '@angular/core';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-level-rename-modal',
  templateUrl: './device-level-rename-modal.component.html',
  styleUrls: ['./device-level-rename-modal.component.css']
})
export class DeviceLevelRenameModalComponent implements OnInit {

  name: string;
  level: number;

  @Output() rename: EventEmitter<[number, string]> = new EventEmitter<[number, string]>();

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  constructor() { }

  ngOnInit() {
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  close() {
    this.rename.emit([this.level, this.name]);
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}
