import { Component, OnInit, ViewChild, ElementRef, Output, EventEmitter } from '@angular/core';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-z-wave-exclude-modal',
  templateUrl: './z-wave-exclude-modal.component.html',
  styleUrls: ['./z-wave-exclude-modal.component.css']
})
export class ZWaveExcludeModalComponent implements OnInit {

  ozw_node_id: string;

  displayezwd_waiting = false;
  displayezwd_result = false;

  @ViewChild('modal', { static: false }) modalRef: ElementRef;

  @Output() abort: EventEmitter<void> = new EventEmitter<void>();
  @Output() close: EventEmitter<void> = new EventEmitter<void>();

  constructor() { }

  ngOnInit() {
    this.ozw_node_id = '-';
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

  OnZWaveAbortExclude() {
    this.abort.emit();
  }

  OnZWaveCloseExclude() {
    this.dismiss();
    this.close.emit();
  }

}
