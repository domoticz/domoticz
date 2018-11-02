import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {FindlatlongDialogComponent} from './findlatlong-dialog.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('FindlatlongDialogComponent', () => {
  let component: FindlatlongDialogComponent;
  let fixture: ComponentFixture<FindlatlongDialogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        {
          provide: DIALOG_DATA,
          useValue: {}
        }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FindlatlongDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
