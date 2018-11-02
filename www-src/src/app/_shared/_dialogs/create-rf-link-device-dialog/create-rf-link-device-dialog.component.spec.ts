import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CreateRfLinkDeviceDialogComponent} from './create-rf-link-device-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';

describe('CreateRfLinkDeviceDialogComponent', () => {
  let component: CreateRfLinkDeviceDialogComponent;
  let fixture: ComponentFixture<CreateRfLinkDeviceDialogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: DIALOG_DATA,
          useValue: {idx: '123'}
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {

    fixture = TestBed.createComponent(CreateRfLinkDeviceDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
