import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {AlertareaComponent} from './alertarea.component';
import {CommonTestModule} from '../../../_testing/common.test.module';

describe('AlertareaComponent', () => {
  let component: AlertareaComponent;
  let fixture: ComponentFixture<AlertareaComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AlertareaComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
