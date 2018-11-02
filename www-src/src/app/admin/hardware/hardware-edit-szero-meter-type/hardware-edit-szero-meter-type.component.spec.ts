import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditSzeroMeterTypeComponent } from './hardware-edit-szero-meter-type.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareEditSzeroMeterTypeComponent', () => {
  let component: HardwareEditSzeroMeterTypeComponent;
  let fixture: ComponentFixture<HardwareEditSzeroMeterTypeComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareEditSzeroMeterTypeComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditSzeroMeterTypeComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
