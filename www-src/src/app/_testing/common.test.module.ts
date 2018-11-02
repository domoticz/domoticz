import {NgModule} from '@angular/core';
import {I18NextModule} from 'angular-i18next';
import {HttpClientTestingModule} from '@angular/common/http/testing';
import {RouterTestingModule} from '@angular/router/testing';
import {SharedModule} from '../_shared/shared.module';

@NgModule({
  declarations: [],
  imports: [
    /* Domoticz */
    SharedModule.forRoot(),
    /* Angular */
    RouterTestingModule,
    HttpClientTestingModule,
    /* 3rd party */
    I18NextModule.forRoot(),
  ],
  providers: [],
  exports: [
    SharedModule,
    RouterTestingModule,
    HttpClientTestingModule,
    I18NextModule
  ]
})
export class CommonTestModule {
}
