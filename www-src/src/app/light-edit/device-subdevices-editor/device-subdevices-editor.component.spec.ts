import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceSubdevicesEditorComponent } from './device-subdevices-editor.component';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { FormsModule } from '@angular/forms';

describe('DeviceSubdevicesEditorComponent', () => {
  let component: DeviceSubdevicesEditorComponent;
  let fixture: ComponentFixture<DeviceSubdevicesEditorComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DeviceSubdevicesEditorComponent ],
      imports: [CommonTestModule, FormsModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceSubdevicesEditorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
