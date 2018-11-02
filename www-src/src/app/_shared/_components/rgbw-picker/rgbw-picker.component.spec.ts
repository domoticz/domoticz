import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RgbwPickerComponent} from './rgbw-picker.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('RgbwPickerComponent', () => {
  let component: RgbwPickerComponent;
  let fixture: ComponentFixture<RgbwPickerComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RgbwPickerComponent);
    component = fixture.componentInstance;

    component.colorSettingsType = '';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
