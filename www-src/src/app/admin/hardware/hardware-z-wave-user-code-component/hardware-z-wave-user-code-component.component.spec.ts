import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareZWaveUserCodeComponentComponent } from './hardware-z-wave-user-code-component.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { ActivatedRoute, convertToParamMap } from '@angular/router';
import {HardwareModule} from "../hardware.module";

describe('HardwareZWaveUserCodeComponentComponent', () => {
  let component: HardwareZWaveUserCodeComponentComponent;
  let fixture: ComponentFixture<HardwareZWaveUserCodeComponentComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [  ],
      imports: [CommonTestModule, HardwareModule],
      providers: [
        {
          provide: ActivatedRoute,
          useValue: {
            snapshot: {
              paramMap: convertToParamMap({ idx: '1', nodeidx: '123' })
            }
          }
        }
      ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HardwareZWaveUserCodeComponentComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
