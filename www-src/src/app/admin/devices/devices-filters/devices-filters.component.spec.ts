import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DevicesFiltersComponent } from './devices-filters.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DevicesFiltersComponent', () => {
  let component: DevicesFiltersComponent;
  let fixture: ComponentFixture<DevicesFiltersComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DevicesFiltersComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DevicesFiltersComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
