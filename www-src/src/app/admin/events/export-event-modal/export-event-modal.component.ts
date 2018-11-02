import {Component, ElementRef, EventEmitter, OnInit, Output, ViewChild} from '@angular/core';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-export-event-modal',
  templateUrl: './export-event-modal.component.html',
  styleUrls: ['./export-event-modal.component.css']
})
export class ExportEventModalComponent implements OnInit {

  scriptData = '';

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  constructor() {
  }

  ngOnInit() {
  }

  open(scriptData: string) {
    this.scriptData = scriptData;
    $(this.modalRef.nativeElement).modal('show');
  }

  close() {
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }


}
