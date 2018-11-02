import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RfxcomFirmwareComponent } from './rfxcom-firmware.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { RoundProgressModule } from 'angular-svg-round-progressbar';

describe('RfxcomFirmwareComponent', () => {
  let component: RfxcomFirmwareComponent;
  let fixture: ComponentFixture<RfxcomFirmwareComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RfxcomFirmwareComponent],
      imports: [CommonTestModule, RoundProgressModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RfxcomFirmwareComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
