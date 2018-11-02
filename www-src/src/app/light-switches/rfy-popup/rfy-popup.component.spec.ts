import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { RfyPopupComponent } from './rfy-popup.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';

describe('RfyPopupComponent', () => {
  let component: RfyPopupComponent;
  let fixture: ComponentFixture<RfyPopupComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ RfyPopupComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(RfyPopupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
