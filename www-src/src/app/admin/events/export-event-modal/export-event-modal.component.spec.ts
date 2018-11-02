import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {ExportEventModalComponent} from './export-event-modal.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';

describe('ExportEventModalComponent', () => {
  let component: ExportEventModalComponent;
  let fixture: ComponentFixture<ExportEventModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ExportEventModalComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ExportEventModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
