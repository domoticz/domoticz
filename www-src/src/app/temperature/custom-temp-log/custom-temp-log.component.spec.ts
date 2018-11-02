import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { CustomTempLogComponent } from './custom-temp-log.component';
import { CommonTestModule } from '../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('CustomTempLogComponent', () => {
  let component: CustomTempLogComponent;
  let fixture: ComponentFixture<CustomTempLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [CustomTempLogComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(CustomTempLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
