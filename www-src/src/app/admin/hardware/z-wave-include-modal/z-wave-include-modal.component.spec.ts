import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ZWaveIncludeModalComponent } from './z-wave-include-modal.component';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('ZWaveIncludeModalComponent', () => {
  let component: ZWaveIncludeModalComponent;
  let fixture: ComponentFixture<ZWaveIncludeModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ZWaveIncludeModalComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ZWaveIncludeModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
