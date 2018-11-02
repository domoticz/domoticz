import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {MediaRemoteDialogComponent} from './media-remote-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('MediaRemoteDialogComponent', () => {
  let component: MediaRemoteDialogComponent;
  let fixture: ComponentFixture<MediaRemoteDialogComponent>;

  const dialogData = {
    Name: '',
    devIdx: '1',
    HWType: ''
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
    fixture = TestBed.createComponent(MediaRemoteDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
