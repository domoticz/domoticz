import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {ScenesDashboardComponent} from './scenes-dashboard.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {SceneWidgetComponent} from '../scene-widget/scene-widget.component';

describe('ScenesDashboardComponent', () => {
  let component: ScenesDashboardComponent;
  let fixture: ComponentFixture<ScenesDashboardComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        ScenesDashboardComponent,
        SceneWidgetComponent
      ],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ScenesDashboardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
