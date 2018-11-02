import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditCcusbComponent } from './hardware-edit-ccusb.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareEditCcusbComponent', () => {
  let component: HardwareEditCcusbComponent;
  let fixture: ComponentFixture<HardwareEditCcusbComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareEditCcusbComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditCcusbComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
