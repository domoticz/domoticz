import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { FloorplansComponent } from './floorplans.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('FloorplansComponent', () => {
  let component: FloorplansComponent;
  let fixture: ComponentFixture<FloorplansComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [FloorplansComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FloorplansComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
