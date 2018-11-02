import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ZWaveExcludeModalComponent } from './z-wave-exclude-modal.component';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('ZWaveExcludeModalComponent', () => {
  let component: ZWaveExcludeModalComponent;
  let fixture: ComponentFixture<ZWaveExcludeModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ZWaveExcludeModalComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ZWaveExcludeModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
