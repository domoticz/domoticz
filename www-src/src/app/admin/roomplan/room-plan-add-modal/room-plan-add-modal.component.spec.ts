import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlanAddModalComponent} from './room-plan-add-modal.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from "@angular/forms";

describe('RoomPlanAddModalComponent', () => {
  let component: RoomPlanAddModalComponent;
  let fixture: ComponentFixture<RoomPlanAddModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RoomPlanAddModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlanAddModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
