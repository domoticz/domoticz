import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {TemperatureComponent} from './temperature.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {TemperatureWidgetComponent} from 'src/app/temperature/temperature-widget/temperature-widget.component';
import {FormsModule} from '@angular/forms';

describe('TemperatureComponent', () => {
  let component: TemperatureComponent;
  let fixture: ComponentFixture<TemperatureComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        TemperatureComponent,
        TemperatureWidgetComponent,
      ],
      imports: [
        CommonTestModule,
        FormsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TemperatureComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
