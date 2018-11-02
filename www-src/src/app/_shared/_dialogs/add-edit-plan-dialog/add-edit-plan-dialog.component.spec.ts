import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AddEditPlanDialogComponent} from './add-edit-plan-dialog.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('AddEditPlanDialogComponent', () => {
  let component: AddEditPlanDialogComponent;
  let fixture: ComponentFixture<AddEditPlanDialogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: DIALOG_DATA,
          useValue: {
            title: 'Add New Plan',
            buttonTitle: 'Add',
            callbackFn: (name: string) => {
              console.log(name);
            }
          }
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AddEditPlanDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
