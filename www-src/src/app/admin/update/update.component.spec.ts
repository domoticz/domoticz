import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {UpdateComponent} from './update.component';
import {CommonTestModule} from "../../_testing/common.test.module";
import {RoundProgressModule} from "angular-svg-round-progressbar";

describe('UpdateComponent', () => {
  let component: UpdateComponent;
  let fixture: ComponentFixture<UpdateComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [UpdateComponent],
      imports: [CommonTestModule, RoundProgressModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UpdateComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
