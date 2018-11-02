import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {ImportEventModalComponent} from './import-event-modal.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from "@angular/forms";

describe('ImportEventModalComponent', () => {
  let component: ImportEventModalComponent;
  let fixture: ComponentFixture<ImportEventModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ImportEventModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ImportEventModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
