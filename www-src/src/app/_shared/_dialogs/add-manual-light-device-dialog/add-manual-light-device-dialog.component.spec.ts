import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AddManualLightDeviceDialogComponent} from './add-manual-light-device-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('AddManualLightDeviceDialogComponent', () => {
  let component: AddManualLightDeviceDialogComponent;
  let fixture: ComponentFixture<AddManualLightDeviceDialogComponent>;

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
    fixture = TestBed.createComponent(AddManualLightDeviceDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
