import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RoomPlansSelectorComponent} from './room-plans-selector.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('RoomPlansSelectorComponent', () => {
  let component: RoomPlansSelectorComponent;
  let fixture: ComponentFixture<RoomPlansSelectorComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [
        CommonTestModule,
        FormsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RoomPlansSelectorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
