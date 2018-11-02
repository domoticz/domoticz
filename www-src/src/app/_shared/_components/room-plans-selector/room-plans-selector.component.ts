import {RoomService} from '../../_services/room.service';
import {Component, EventEmitter, OnInit, Output} from '@angular/core';
import {Plan} from '../../_models/plan';
import {ConfigService} from '../../_services/config.service';

@Component({
  selector: 'dz-room-plans-selector',
  templateUrl: './room-plans-selector.component.html',
  styleUrls: ['./room-plans-selector.component.css']
})
export class RoomPlansSelectorComponent implements OnInit {

  RoomPlans: Array<Plan> = [];
  roomSelected = 0;

  @Output()
  roomSelectedChange: EventEmitter<number> = new EventEmitter<number>();

  constructor(private roomService: RoomService,
              private configService: ConfigService) {
  }

  ngOnInit() {
    this.roomService.getPlans().subscribe(plans => {
      this.RoomPlans = plans;
    });

    this.roomSelected = this.configService.globals.LastPlanSelected;
  }

  public changeRoom() {
    this.configService.globals.LastPlanSelected = this.roomSelected;
    this.roomSelectedChange.emit(this.roomSelected);
  }

}
