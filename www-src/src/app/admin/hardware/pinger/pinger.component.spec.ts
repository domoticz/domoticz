import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { PingerComponent } from './pinger.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import { Hardware } from 'src/app/_shared/_models/hardware';
import {HardwareModule} from "../hardware.module";

describe('PingerComponent', () => {
  let component: PingerComponent;
  let fixture: ComponentFixture<PingerComponent>;

  const HARDWARE: Hardware = {
    Address: '',
    DataTimeout: 0,
    Enabled: 'true',
    Extra: '',
    Mode1: 1,
    Mode2: 2,
    Mode3: 3,
    Mode4: 4,
    Mode5: 5,
    Mode6: 6,
    Name: 'Name',
    Password: '',
    Port: 9000,
    SerialPort: '',
    Type: 1,
    Username: '',
    idx: '1',
    noiselvl: 10,
    version: ''
  };

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, HardwareModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(PingerComponent);
    component = fixture.componentInstance;

    component.hardware = HARDWARE;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
