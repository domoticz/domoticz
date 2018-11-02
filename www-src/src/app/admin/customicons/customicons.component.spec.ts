import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CustomiconsComponent } from './customicons.component';
import { CommonTestModule } from '../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import {CustomiconsModule} from "./customicons.module";

describe('CustomiconsComponent', () => {
  let component: CustomiconsComponent;
  let fixture: ComponentFixture<CustomiconsComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, CustomiconsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CustomiconsComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
