import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditLimitlessTypeComponent } from './hardware-edit-limitless-type.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareEditLimitlessTypeComponent', () => {
  let component: HardwareEditLimitlessTypeComponent;
  let fixture: ComponentFixture<HardwareEditLimitlessTypeComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareEditLimitlessTypeComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditLimitlessTypeComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
