import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CreateDummySensorDialogComponent} from './create-dummy-sensor-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';

describe('CreateDummySensorDialogComponent', () => {
  let component: CreateDummySensorDialogComponent;
  let fixture: ComponentFixture<CreateDummySensorDialogComponent>;

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
    fixture = TestBed.createComponent(CreateDummySensorDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
