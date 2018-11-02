import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AddEditFloorplanDialogComponent} from './add-edit-floorplan-dialog.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('AddEditFloorplanDialogComponent', () => {
  let component: AddEditFloorplanDialogComponent;
  let fixture: ComponentFixture<AddEditFloorplanDialogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: DIALOG_DATA,
          useValue: {
            floorplanname: '',
            scalefactor: '1.0',
            editMode: true,
            callbackFn: (csettings) => {
              console.log(csettings);
            }
          }
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AddEditFloorplanDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
