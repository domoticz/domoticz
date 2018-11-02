import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {HeosPlayerRemoteDialogComponent} from './heos-player-remote-dialog.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {DIALOG_DATA} from '../../_services/dialog.service';

describe('HeosPlayerRemoteDialogComponent', () => {
  let component: HeosPlayerRemoteDialogComponent;
  let fixture: ComponentFixture<HeosPlayerRemoteDialogComponent>;

  const dialogData = {
    Name: '',
    devIdx: ''
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
    fixture = TestBed.createComponent(HeosPlayerRemoteDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
