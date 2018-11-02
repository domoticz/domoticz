import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceFilterByUsageComponent } from './device-filter-by-usage.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DeviceFilterByUsageComponent', () => {
  let component: DeviceFilterByUsageComponent;
  let fixture: ComponentFixture<DeviceFilterByUsageComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceFilterByUsageComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceFilterByUsageComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
