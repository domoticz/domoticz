import {Component, ElementRef, EventEmitter, OnInit, Output, ViewChild} from '@angular/core';
import {RoomService} from '../../../_shared/_services/room.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-room-plan-edit-modal',
  templateUrl: './room-plan-edit-modal.component.html',
  styleUrls: ['./room-plan-edit-modal.component.css']
})
export class RoomPlanEditModalComponent implements OnInit {

  @ViewChild('modal', {static: false}) modalRef: ElementRef;

  planId: string;
  planName: string;
  isSaving = false;

  @Output() updated: EventEmitter<void> = new EventEmitter<void>();

  constructor(private roomService: RoomService) {
  }

  ngOnInit() {
  }

  open(planId: string, planName: string) {
    this.planId = planId;
    this.planName = planName;
    $(this.modalRef.nativeElement).modal('show');
  }

  update() {
    this.isSaving = true;
    this.roomService.updatePlan(this.planId, this.planName).subscribe(() => {
      this.close();
    });
  }

  close() {
    this.updated.emit();
    this.dismiss();
  }

  dismiss() {
    $(this.modalRef.nativeElement).modal('hide');
  }

}
