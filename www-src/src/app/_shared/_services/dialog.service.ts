import { Injectable, ComponentFactoryResolver, ApplicationRef, Injector, ComponentRef, InjectionToken } from '@angular/core';
import {
  ComponentType,
  Portal,
  ComponentPortal,
  DomPortalHost,
  PortalInjector
} from '@angular/cdk/portal';

/** Injection token that can be used to access the data that was passed in to a dialog. */
export const DIALOG_DATA = new InjectionToken<any>('DialogData');

// Inspired by https://hackernoon.com/a-first-look-into-the-angular-cdk-67e68807ed9b
// Adapted for Domoticz by Gaël Jourdan-Weil
@Injectable()
export class DialogService {

  private bodyPortalHost: DomPortalHost;

  private _counter = 0;

  constructor(
    private componentFactoryResolver: ComponentFactoryResolver,
    private appRef: ApplicationRef,
    private injector: Injector) {

    this.bodyPortalHost = new DomPortalHost(document.body, this.componentFactoryResolver, this.appRef, this.injector);
  }

  addDialog<T>(dialogComponentType: ComponentType<T>, dialogData: any): ComponentRef<T> {
    const injectionTokens = new WeakMap<any, any>([
      [DIALOG_DATA, dialogData]
    ]);
    const injector = new PortalInjector(this.injector, injectionTokens);
    const portal: ComponentPortal<T> = new ComponentPortal(dialogComponentType, undefined, injector);
    return this.bodyPortalHost.attachComponentPortal(portal);
  }

  removeDialog(): void {
    this.bodyPortalHost.detach();
  }

  /**
   * Returns the next ID to use for a dialog.
   */
  public get dialogId(): number {
    return this._counter++;
  }

}
