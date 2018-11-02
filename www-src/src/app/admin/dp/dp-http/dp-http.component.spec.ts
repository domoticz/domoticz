import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DpHttpComponent} from './dp-http.component';
import {CommonTestModule} from "../../../_testing/common.test.module";
import {FormsModule} from "@angular/forms";
import {DpHttpModule} from "./dp-http.module";

describe('DpHttpComponent', () => {
  let component: DpHttpComponent;
  let fixture: ComponentFixture<DpHttpComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, DpHttpModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DpHttpComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
