import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {SceneLogComponent} from './scene-log.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('SceneLogComponent', () => {
  let component: SceneLogComponent;
  let fixture: ComponentFixture<SceneLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [SceneLogComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SceneLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
