import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlanEditModalComponent} from './room-plan-edit-modal.component';
import {CommonTestModule} from "../../../_testing/common.test.module";
import {FormsModule} from "@angular/forms";

describe('RoomPlanEditModalComponent', () => {
  let component: RoomPlanEditModalComponent;
  let fixture: ComponentFixture<RoomPlanEditModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlanEditModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlanEditModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
