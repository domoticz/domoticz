import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareComponent } from './hardware.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareComponent', () => {
  let component: HardwareComponent;
  let fixture: ComponentFixture<HardwareComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
