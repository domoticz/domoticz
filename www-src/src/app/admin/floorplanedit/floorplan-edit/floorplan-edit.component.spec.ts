import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {FloorplanEditComponent} from './floorplan-edit.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('FloorplanEditComponent', () => {
  let component: FloorplanEditComponent;
  let fixture: ComponentFixture<FloorplanEditComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [FloorplanEditComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FloorplanEditComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
