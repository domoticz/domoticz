import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DpInfluxComponent} from './dp-influx.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from "@angular/forms";
import {DpInfluxModule} from "./dp-influx.module";

describe('DpInfluxComponent', () => {
  let component: DpInfluxComponent;
  let fixture: ComponentFixture<DpInfluxComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, DpInfluxModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DpInfluxComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
