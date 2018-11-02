import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AddSceneDialogComponent } from './add-scene-dialog.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import { DIALOG_DATA } from 'src/app/_shared/_services/dialog.service';

describe('AddSceneDialogComponent', () => {
  let component: AddSceneDialogComponent;
  let fixture: ComponentFixture<AddSceneDialogComponent>;

  const data = {
    addedCallbackFn: () => {
      console.log('added');
    }
  };

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule],
      providers: [
        { provide: DIALOG_DATA, useValue: data }
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AddSceneDialogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
