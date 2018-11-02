import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AddEditCameraDialogComponent} from './add-edit-camera-dialog.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';

describe('AddEditCameraDialogComponent', () => {
  let component: AddEditCameraDialogComponent;
  let fixture: ComponentFixture<AddEditCameraDialogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: DIALOG_DATA,
          useValue: {mode: 'add'}
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AddEditCameraDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
