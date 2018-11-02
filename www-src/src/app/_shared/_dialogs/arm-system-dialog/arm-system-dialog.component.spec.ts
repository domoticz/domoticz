import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {ArmSystemDialogComponent} from './arm-system-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('ArmSystemDialogComponent', () => {
  let component: ArmSystemDialogComponent;
  let fixture: ComponentFixture<ArmSystemDialogComponent>;

  const dialogData = {
    meiantech: true
  };

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule],
      providers: [
        {provide: DIALOG_DATA, useValue: dialogData}
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ArmSystemDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
