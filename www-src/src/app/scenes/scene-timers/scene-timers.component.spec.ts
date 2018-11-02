import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {SceneTimersComponent} from './scene-timers.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {ScenesModule} from '../scenes.module';

describe('SceneTimersComponent', () => {
  let component: SceneTimersComponent;
  let fixture: ComponentFixture<SceneTimersComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [
        CommonTestModule,
        FormsModule,
        ScenesModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SceneTimersComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
