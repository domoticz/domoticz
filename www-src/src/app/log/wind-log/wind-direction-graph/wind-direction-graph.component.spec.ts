import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { WindDirectionGraphComponent } from './wind-direction-graph.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('WindDirectionGraphComponent', () => {
  let component: WindDirectionGraphComponent;
  let fixture: ComponentFixture<WindDirectionGraphComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [WindDirectionGraphComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(WindDirectionGraphComponent);
    component = fixture.componentInstance;

    component.idx = '1';

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
