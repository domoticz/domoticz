import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AddAriluxDialogComponent} from './add-arilux-dialog.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';
import {FormsModule} from '@angular/forms';

describe('AddAriluxDialogComponent', () => {
  let component: AddAriluxDialogComponent;
  let fixture: ComponentFixture<AddAriluxDialogComponent>;

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
    fixture = TestBed.createComponent(AddAriluxDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
