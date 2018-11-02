import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CameraLiveDialogComponent} from './camera-live-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('CameraLiveDialogComponent', () => {
  let component: CameraLiveDialogComponent;
  let fixture: ComponentFixture<CameraLiveDialogComponent>;

  const dialogData = {
    item: {}
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
    fixture = TestBed.createComponent(CameraLiveDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
