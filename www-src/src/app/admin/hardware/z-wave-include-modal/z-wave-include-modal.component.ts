import { Component, OnInit, ViewChild, ElementRef, Output, EventEmitter } from '@angular/core';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-z-wave-include-modal',
  templateUrl: './z-wave-include-modal.component.html',
  styleUrls: ['./z-wave-include-modal.component.css']
})
export class ZWaveIncludeModalComponent implements OnInit {

  ozw_node_id: string;
  ozw_node_desc: string;

  displayizwd_waiting = false;
  displayizwd_result = false;

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  @Output() abort: EventEmitter<void> = new EventEmitter<void>();
  @Output() close: EventEmitter<void> = new EventEmitter<void>();

  constructor() { }

  ngOnInit() {
    this.ozw_node_id = '-';
    this.ozw_node_desc = '-';
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

  OnZWaveAbortInclude() {
    this.abort.emit();
  }

  OnZWaveCloseInclude() {
    this.dismiss();
    this.close.emit();
  }

}
