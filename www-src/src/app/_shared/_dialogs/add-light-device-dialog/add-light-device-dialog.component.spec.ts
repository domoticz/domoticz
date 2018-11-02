import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AddLightDeviceDialogComponent} from './add-light-device-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';

describe('AddLightDeviceDialogComponent', () => {
  let component: AddLightDeviceDialogComponent;
  let fixture: ComponentFixture<AddLightDeviceDialogComponent>;

  const dialogData = {
    devIdx: '',
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
    fixture = TestBed.createComponent(AddLightDeviceDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
