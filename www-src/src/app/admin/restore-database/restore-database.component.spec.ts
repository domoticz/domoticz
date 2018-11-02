import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {RestoreDatabaseComponent} from './restore-database.component';
import {CommonTestModule} from "../../_testing/common.test.module";
import {FormsModule} from "@angular/forms";

describe('RestoreDatabaseComponent', () => {
  let component: RestoreDatabaseComponent;
  let fixture: ComponentFixture<RestoreDatabaseComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [RestoreDatabaseComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RestoreDatabaseComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
