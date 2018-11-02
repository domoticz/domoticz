import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DpFibaroComponent} from './dp-fibaro.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DpFibaroModule} from "./dp-fibaro.module";

describe('DpFibaroComponent', () => {
  let component: DpFibaroComponent;
  let fixture: ComponentFixture<DpFibaroComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, DpFibaroModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DpFibaroComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
