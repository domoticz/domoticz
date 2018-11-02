import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DpGooglePubSubComponent} from './dp-google-pub-sub.component';
import {CommonTestModule} from '../../../_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DpGooglePubSubModule} from "./dp-google-pub-sub.module";

describe('DpGooglePubSubComponent', () => {
  let component: DpGooglePubSubComponent;
  let fixture: ComponentFixture<DpGooglePubSubComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule, FormsModule, DpGooglePubSubModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DpGooglePubSubComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
