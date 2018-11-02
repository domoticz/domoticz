import {EditStateDialogComponent} from '../../_shared/_dialogs/edit-state-dialog/edit-state-dialog.component';
import {EditSetPointDialogComponent} from '../../_shared/_dialogs/edit-set-point-dialog/edit-set-point-dialog.component';
import {DialogService} from '../../_shared/_services/dialog.service';
import {DeviceService} from '../../_shared/_services/device.service';
import {ConfigService} from '../../_shared/_services/config.service';
import {PermissionService} from '../../_shared/_services/permission.service';
import {Component, EventEmitter, Inject, Input, OnInit, Output} from '@angular/core';
import {Utils} from 'src/app/_shared/_utils/utils';
import {EditTempDeviceDialogComponent} from 'src/app/_shared/_dialogs/edit-temp-device-dialog/edit-temp-device-dialog.component';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {timer} from 'rxjs';
import {NotificationService} from '../../_shared/_services/notification.service';
import {Device} from '../../_shared/_models/device';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Component({
  selector: 'dz-temperature-widget',
  templateUrl: './temperature-widget.component.html',
  styleUrls: ['./temperature-widget.component.css']
})
export class TemperatureWidgetComponent implements OnInit {

  @Input()
  item: Device;

  @Output()
  private replaceDevice: EventEmitter<void> = new EventEmitter<void>();
  @Output()
  private removeDevice: EventEmitter<void> = new EventEmitter<void>();

  highlight = false;

  tempsign: string;
  windsign: string;

  displayTrend: boolean;
  trendState?: string;

  constructor(
    private permissionService: PermissionService,
    private configService: ConfigService,
    private deviceService: DeviceService,
    private notificationService: NotificationService,
    private dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
  }

  ngOnInit() {
    // Trigger highlight effect when creating the component
    // (the component get recreated when RefreshTemp is called on temperature page)
    this.highlightItem();

    this.tempsign = this.configService.config.TempSign;
    this.windsign = this.configService.config.WindSign;

    this.displayTrend = Utils.DisplayTrend(this.item.trend);
    if (this.displayTrend) {
      this.trendState = Utils.TrendState(this.item.trend);
    }
  }

  get nbackstyle(): string {
    let backgroundClass = Utils.GetItemBackgroundStatus(this.item);
    if (this.displaySetPoint()) {
      if (this.sHeatMode() === 'HeatingOff' || !this.isSetPointOn()) { // seems to be used whenever the heating is off
        backgroundClass = 'statusEvoSetPointOff';
      } else if (this.item.SetPoint >= 25) {
        backgroundClass = 'statusEvoSetPoint25';
      } else if (this.item.SetPoint >= 22) {
        backgroundClass = 'statusEvoSetPoint22';
      } else if (this.item.SetPoint >= 19) {
        backgroundClass = 'statusEvoSetPoint19';
      } else if (this.item.SetPoint >= 16) {
        backgroundClass = 'statusEvoSetPoint16';
      } else { // min on temp 5 or greater
        backgroundClass = 'statusEvoSetPointMin';
      }
    }
    return backgroundClass;
  }

  private highlightItem() {
    if (this.configService.config.ShowUpdatedEffect === true) {
      this.highlight = true;
      timer(1000).subscribe(_ => this.highlight = false);
    }
  }

  private sHeatMode(): string {
    if (this.item.Status) { // FIXME only support this for evohome?
      return this.item.Status;
    } else {
      return '';
    }
  }

  public displaySetPoint(): boolean {
    return (this.item.SubType === 'Zone' || this.item.SubType === 'Hot Water') && typeof this.item.SetPoint !== 'undefined';
  }

  public isSetPointOn(): boolean {
    return this.item.SetPoint !== 325.1;
  }

  public displayState(): boolean {
    return (this.item.SubType === 'Zone' || this.item.SubType === 'Hot Water') && typeof this.item.State !== 'undefined';
  }

  public displayHeat(): boolean {
    return (this.item.SubType === 'Zone' || this.item.SubType === 'Hot Water') &&
      this.sHeatMode() !== 'Auto' && this.sHeatMode() !== 'FollowSchedule';
  }

  public imgHeat(): string | undefined {
    if (this.displayHeat()) {
      return this.sHeatMode() + ((this.item.SubType === 'Hot Water') ? 'Inv' : '');
    } else {
      return undefined;
    }
  }

  public image(): string {
    if (typeof this.item.Temp !== 'undefined') {
      return this.deviceService.GetTemp48Item(this.item.Temp);
    } else {
      if (this.item.Type === 'Humidity') {
        return 'gauge48.png';
      } else {
        return this.deviceService.GetTemp48Item(this.item.Chill);
      }
    }
  }

  public displayMode(): boolean {
    return (this.item.SubType === 'Zone' || this.item.SubType === 'Hot Water');
  }

  public EvoDisplayTextMode(): string {
    return Utils.EvoDisplayTextMode(this.sHeatMode());
  }

  public displayUntil(): boolean {
    return (this.item.SubType === 'Zone' || this.item.SubType === 'Hot Water') && typeof this.item.Until !== 'undefined';
  }

  public dtUntil(): string {
    if (this.item.Until) {
      // const tUntil = this.item.Until.replace(/Z/, '').replace(/\..+/, '') + 'Z';
      // let dtUntil = new Date(tUntil);
      // dtUntil = new Date(dtUntil.getTime() - dtUntil.getTimezoneOffset() * 60000);
      // return dtUntil.toISOString().replace(/T/, ' ').replace(/\..+/, '');
      return this.item.Until.replace(/T/, ' ').replace(/\..+/, '');
    }
  }

  public displayGust(): boolean {
    return this.item.Direction !== undefined && typeof this.item.Gust !== 'undefined';
  }

  public MakeFavorite(isfavorite: number): void {
    this.deviceService.makeFavorite(Number(this.item.idx), isfavorite).subscribe(_ => {
      this.item.Favorite = isfavorite;
    });
  }

  public EditTempDevice(displayAddj: boolean): void {
    const dialogData = {
      item: Object.assign({}, this.item), // Copy item so that the dialog has its own editable instance
      tempsign: this.tempsign,
      replaceCallbackFn: () => {
        this.replaceDevice.emit(undefined);
      },
      removeCallbackFn: () => {
        this.removeDevice.emit(undefined);
      },
      updateCallbackFn: (updatedItem: Device) => {
        this.item = updatedItem;
      },
      displayAddj: displayAddj
    };
    const dialog = this.dialogService.addDialog(EditTempDeviceDialogComponent, dialogData);
    dialog.instance.init();
    dialog.instance.open();
  }

  public EditSetPoint() {
    const mode = this.item.Status;

    // HeatingOff does not apply to dhw
    if (mode === 'HeatingOff') {
      bootbox.alert(this.translationService.t('Can\'t change zone when the heating is off'));
      return false;
    }

    const data = {
      item: Object.assign({}, this.item), // Copy item so that dialog has its own copy
      updateCallbackFn: (updatedItem) => {
        this.item = updatedItem;
      },
      displayCancelOverride: mode && mode.indexOf('Override') > -1
    };

    const dialog = this.dialogService.addDialog(EditSetPointDialogComponent, data).instance;
    dialog.init();
    dialog.open();
  }

  public EditState(): void {
    // HeatingOff does not apply to dhw

    const data = {
      item: Object.assign({}, this.item), // Copy item so that dialog has its own copy
      updateCallbackFn: (updatedItem) => {
        this.item = updatedItem;
      }
    };

    const dialog = this.dialogService.addDialog(EditStateDialogComponent, data).instance;
    dialog.init();
    dialog.open();
  }

}
