import {Component, ElementRef, EventEmitter, OnInit, Output, ViewChild} from '@angular/core';
import {RoomService} from '../../../_shared/_services/room.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-room-plan-add-modal',
  templateUrl: './room-plan-add-modal.component.html',
  styleUrls: ['./room-plan-add-modal.component.css']
})
export class RoomPlanAddModalComponent implements OnInit {

  @ViewChild('modal', {static: false}) modalRef: ElementRef;

  planName: string;
  isSaving = false;

  @Output() added: EventEmitter<void> = new EventEmitter<void>();

  constructor(private roomService: RoomService) {
  }

  ngOnInit() {
  }

  open() {
    $(this.modalRef.nativeElement).modal('show');
  }

  add() {
    this.isSaving = true;
    this.roomService.addPlan(this.planName).subscribe(() => {
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
