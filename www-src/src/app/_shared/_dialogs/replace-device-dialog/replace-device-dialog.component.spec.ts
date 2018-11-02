import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from '../../_services/dialog.service';
import {ReplaceDeviceDialogComponent} from './replace-device-dialog.component';

describe('ReplaceDevicDialogComponent', () => {
  let component: ReplaceDeviceDialogComponent;
  let fixture: ComponentFixture<ReplaceDeviceDialogComponent>;

  const dialogData = {
    addCallback: () => {
    }
  };

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {provide: DIALOG_DATA, useValue: dialogData}
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ReplaceDeviceDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
