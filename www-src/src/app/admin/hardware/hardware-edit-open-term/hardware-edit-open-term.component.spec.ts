import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditOpenTermComponent } from './hardware-edit-open-term.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('HardwareEditOpenTermComponent', () => {
  let component: HardwareEditOpenTermComponent;
  let fixture: ComponentFixture<HardwareEditOpenTermComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareEditOpenTermComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditOpenTermComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
