import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { EditRego6xxTypeComponent } from './edit-rego6xx-type.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('EditRego6xxTypeComponent', () => {
  let component: EditRego6xxTypeComponent;
  let fixture: ComponentFixture<EditRego6xxTypeComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ EditRego6xxTypeComponent ],
      imports: [CommonTestModule,FormsModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(EditRego6xxTypeComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
