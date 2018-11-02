import {Component, EventEmitter, Inject, Input, OnInit, Output} from '@angular/core';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ConfigService} from '../../_shared/_services/config.service';
import {PermissionService} from '../../_shared/_services/permission.service';
import {DeviceService} from '../../_shared/_services/device.service';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Router} from '@angular/router';
import {DialogService} from '../../_shared/_services/dialog.service';
import {EditRainDeviceDialogComponent} from '../../_shared/_dialogs/edit-rain-device-dialog/edit-rain-device-dialog.component';
import {EditVisibilityDeviceDialogComponent} from '../../_shared/_dialogs/edit-visibility-device-dialog/edit-visibility-device-dialog.component';
import {EditBaroDeviceDialogComponent} from '../../_shared/_dialogs/edit-baro-device-dialog/edit-baro-device-dialog.component';
import {EditWeatherDeviceDialogComponent} from '../../_shared/_dialogs/edit-weather-device-dialog/edit-weather-device-dialog.component';
import {timer} from 'rxjs';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: 'dz-weather-widget',
  templateUrl: './weather-widget.component.html',
  styleUrls: ['./weather-widget.component.css']
})
export class WeatherWidgetComponent implements OnInit {

  @Input() item: Device;

  @Output() private replaceDevice: EventEmitter<void> = new EventEmitter<void>();
  @Output() private removeDevice: EventEmitter<void> = new EventEmitter<void>();

  windsign: string;
  tempsign: string;

  highlight = false;

  constructor(
    private configService: ConfigService,
    private permissionService: PermissionService,
    private deviceService: DeviceService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private router: Router,
    private dialogService: DialogService
  ) {
  }

  ngOnInit() {
    // Trigger highlight effect when creating the component
    // (the component get recreated when RefreshWeather is called on dashboard page)
    this.highlightItem();

    this.windsign = this.configService.config.WindSign;
    this.tempsign = this.configService.config.TempSign;
  }

  private highlightItem() {
    if (this.configService.config.ShowUpdatedEffect === true) {
      this.highlight = true;
      timer(1000).subscribe(_ => this.highlight = false);
    }
  }

  nbackstyle(): string {
    return Utils.GetItemBackgroundStatus(this.item);
  }

  image(): string {
    if (typeof this.item.Barometer !== 'undefined') {
      return 'baro48.png';
    } else if (typeof this.item.Rain !== 'undefined') {
      return 'Rain48_On.png';
    } else if (typeof this.item.Visibility !== 'undefined') {
      return 'visibility48.png';
    } else if (typeof this.item.UVI !== 'undefined') {
      return 'uv48.png';
    } else if (typeof this.item.Radiation !== 'undefined') {
      return 'radiation48.png';
    } else if (typeof this.item.Direction !== 'undefined') {
      return 'Wind' + this.item.DirectionStr + '.png';
    }
  }

  MakeFavorite(isfavorite: number): void {
    this.deviceService.makeFavorite(Number(this.item.idx), isfavorite).subscribe(_ => {
      this.item.Favorite = isfavorite;
    });
  }

  ShowLog() {
    if (typeof this.item.Barometer !== 'undefined') {
      this.router.navigate(['/BaroLog', this.item.idx]);
    } else if (typeof this.item.Rain !== 'undefined') {
      this.router.navigate(['/RainLog', this.item.idx]);
    } else if (typeof this.item.UVI !== 'undefined') {
      this.router.navigate(['/UVLog', this.item.idx]);
    } else if (typeof this.item.Direction !== 'undefined') {
      this.router.navigate(['/WindLog', this.item.idx]);
    } else if (typeof this.item.Visibility !== 'undefined') {
      this.router.navigate(['/Devices', this.item.idx, 'Log']);
    } else if (typeof this.item.Radiation !== 'undefined') {
      this.router.navigate(['/Devices', this.item.idx, 'Log']);
    }
  }

  EditDevice() {
    let dialog;
    const dialogOptions = {
      item: Object.assign({}, this.item), // Copy item so that the dialog has its own editable instance
      replaceCallbackFn: () => {
        this.replaceDevice.emit(undefined);
      },
      removeCallbackFn: () => {
        this.removeDevice.emit(undefined);
      },
      updateCallbackFn: (updatedItem: Device) => {
        this.item = updatedItem;
      }
    };

    if (typeof this.item.Rain !== 'undefined') {
      dialog = this.dialogService.addDialog(EditRainDeviceDialogComponent, dialogOptions);
    } else if (typeof this.item.Visibility !== 'undefined') {
      dialog = this.dialogService.addDialog(EditVisibilityDeviceDialogComponent, dialogOptions);
    } else if (typeof this.item.Barometer !== 'undefined') {
      dialog = this.dialogService.addDialog(EditBaroDeviceDialogComponent, dialogOptions);
    } else {
      dialog = this.dialogService.addDialog(EditWeatherDeviceDialogComponent, dialogOptions);
    }

    if (dialog) {
      dialog.instance.init();
      dialog.instance.open();
    }
  }

}
