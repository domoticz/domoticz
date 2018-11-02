import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HardwareEditLmsComponent } from './hardware-edit-lms.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import { ActivatedRoute, convertToParamMap } from '@angular/router';
import { HardwareService } from '../../../_shared/_services/hardware.service';
import { Hardware } from 'src/app/_shared/_models/hardware';
import { of } from 'rxjs';

describe('HardwareEditLmsComponent', () => {
  let component: HardwareEditLmsComponent;
  let fixture: ComponentFixture<HardwareEditLmsComponent>;
  let hardwareService: HardwareService;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [HardwareEditLmsComponent],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: ActivatedRoute,
          useValue: {
            snapshot: {
              paramMap: convertToParamMap({ idx: '1' })
            }
          }
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    hardwareService = TestBed.get(HardwareService);
    spyOn(hardwareService, 'getHardware').and.returnValue(of({ idx: '123', Mode1: 20 } as Hardware));

    fixture = TestBed.createComponent(HardwareEditLmsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
