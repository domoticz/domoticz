import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {LightSwitchesDashboardComponent} from './light-switches-dashboard.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {LightSwitchWidgetComponent} from '../light-switch-widget/light-switch-widget.component';
import {FormsModule} from '@angular/forms';
import {RfyPopupComponent} from '../rfy-popup/rfy-popup.component';

describe('LightSwitchesDashboardComponent', () => {
  let component: LightSwitchesDashboardComponent;
  let fixture: ComponentFixture<LightSwitchesDashboardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        LightSwitchesDashboardComponent,
        LightSwitchWidgetComponent,
        RfyPopupComponent,
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(LightSwitchesDashboardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
