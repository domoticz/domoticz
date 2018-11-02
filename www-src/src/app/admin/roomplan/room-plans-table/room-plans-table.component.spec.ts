import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlansTableComponent} from './room-plans-table.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {RoomPlanEditModalComponent} from "../room-plan-edit-modal/room-plan-edit-modal.component";
import {FormsModule} from "@angular/forms";

describe('RoomPlansTableComponent', () => {
  let component: RoomPlansTableComponent;
  let fixture: ComponentFixture<RoomPlansTableComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlansTableComponent, RoomPlanEditModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlansTableComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
