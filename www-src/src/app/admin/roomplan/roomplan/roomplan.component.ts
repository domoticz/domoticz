import {Component, Inject, OnInit, ViewChild} from '@angular/core';
import {RoomService} from '../../../_shared/_services/room.service';
import {Plan, PlanDevice} from '../../../_shared/_models/plan';
import {PermissionService} from '../../../_shared/_services/permission.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {RoomPlanAddModalComponent} from '../room-plan-add-modal/room-plan-add-modal.component';
import {RoomPlanDeviceSelectorModalComponent} from '../room-plan-device-selector-modal/room-plan-device-selector-modal.component';
import {mergeMap} from 'rxjs/operators';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-roomplan',
  templateUrl: './roomplan.component.html',
  styleUrls: ['./roomplan.component.css']
})
export class RoomplanComponent implements OnInit {

  @ViewChild(RoomPlanAddModalComponent, {static: false}) addModal: RoomPlanAddModalComponent;
  @ViewChild(RoomPlanDeviceSelectorModalComponent, {static: false}) selectDeviceModal: RoomPlanDeviceSelectorModalComponent;

  plans: Array<Plan> = [];
  planDevices: Array<PlanDevice> = [];

  selectedPlan: Plan = undefined;

  constructor(private roomService: RoomService,
              private permissionService: PermissionService,
              private notificationService: NotificationService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
  }

  ngOnInit() {
    this.refreshPlans();
  }

  public refreshPlans() {
    this.roomService.getPlansWithHidden().subscribe(plans => {
      this.plans = plans;
    });
  }

  public refreshPlanDevices() {
    if (!this.selectedPlan) {
      this.planDevices = [];
      return;
    }

    this.roomService.getPlanDevices(this.selectedPlan.idx).subscribe(devices => {
      this.planDevices = devices;
    });
  }

  public selectPlan(plan: Plan) {
    this.selectedPlan = plan;
    this.refreshPlanDevices();
  }

  public removeAllDevicesFromPlan() {
    bootbox.confirm(this.translationService.t('Are you sure to delete ALL Active Devices?\n\nThis action can not be undone!!'),
      (result: boolean) => {
        if (result === true) {
          this.roomService.removeAllDevicesFromPlan(this.selectedPlan.idx).subscribe(() => {
            this.refreshPlanDevices();
          });
        }
      });
  }

  addPlan() {
    this.addModal.open();
    this.addModal.added.subscribe(() => {
      this.refreshPlans();
    });
  }

  addDeviceToPlan() {
    this.selectDeviceModal.open();
    this.selectDeviceModal.selected.pipe(
      mergeMap((selectedDevice) => {
        return this.roomService.addDeviceToPlan(this.selectedPlan.idx, selectedDevice.idx, selectedDevice.type === 1);
      })
    ).subscribe(() => {
      this.refreshPlanDevices();
    });
  }

}
