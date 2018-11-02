import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CreateEvohomeSensorsDialogComponent} from './create-evohome-sensors-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DIALOG_DATA} from 'src/app/_shared/_services/dialog.service';

describe('CreateEvohomeSensorsDialogComponent', () => {
  let component: CreateEvohomeSensorsDialogComponent;
  let fixture: ComponentFixture<CreateEvohomeSensorsDialogComponent>;

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
    fixture = TestBed.createComponent(CreateEvohomeSensorsDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
