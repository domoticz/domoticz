import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {UtilityDashboardComponent} from './utility-dashboard.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {UtilityWidgetComponent} from '../utility-widget/utility-widget.component';
import {FormsModule} from '@angular/forms';

describe('UtilityDashboardComponent', () => {
  let component: UtilityDashboardComponent;
  let fixture: ComponentFixture<UtilityDashboardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        UtilityDashboardComponent,
        UtilityWidgetComponent,
      ],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(UtilityDashboardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
