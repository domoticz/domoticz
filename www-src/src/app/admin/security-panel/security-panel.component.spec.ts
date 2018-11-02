import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {SecurityPanelComponent} from './security-panel.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {SecurityPanelModule} from "./security-panel.module";

describe('SecurityPanelComponent', () => {
  let component: SecurityPanelComponent;
  let fixture: ComponentFixture<SecurityPanelComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, SecurityPanelModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SecurityPanelComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
