import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditSbfSpotComponent } from './hardware-edit-sbf-spot.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('HardwareEditSbfSpotComponent', () => {
  let component: HardwareEditSbfSpotComponent;
  let fixture: ComponentFixture<HardwareEditSbfSpotComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ HardwareEditSbfSpotComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareEditSbfSpotComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
