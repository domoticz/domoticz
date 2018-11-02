import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {EditSceneComponent} from './edit-scene.component';
import {CommonTestModule} from '../../_testing/common.test.module';
import {EditSceneDevicesTableComponent} from '../edit-scene-devices-table/edit-scene-devices-table.component';
import {EditSceneActivationTableComponent} from '../edit-scene-activation-table/edit-scene-activation-table.component';
import {FormsModule} from '@angular/forms';

describe('EditSceneComponent', () => {
  let component: EditSceneComponent;
  let fixture: ComponentFixture<EditSceneComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        EditSceneComponent,
        EditSceneActivationTableComponent,
        EditSceneDevicesTableComponent,
      ],
      imports: [
        CommonTestModule,
        FormsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(EditSceneComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
