import {Component, ElementRef, EventEmitter, OnInit, Output, ViewChild} from '@angular/core';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-import-event-modal',
  templateUrl: './import-event-modal.component.html',
  styleUrls: ['./import-event-modal.component.css']
})
export class ImportEventModalComponent implements OnInit {

  scriptData = '';

  @Output() import: EventEmitter<string> = new EventEmitter<string>();

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  constructor() {
  }

  ngOnInit() {
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  close() {
    this.import.emit(this.scriptData);
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}
