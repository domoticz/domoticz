import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { Component, OnInit, Input, Output, EventEmitter, Inject, ViewChild, OnDestroy, Type } from '@angular/core';
import { Utils } from 'src/app/_shared/_utils/utils';
import { ConfigService } from '../../_shared/_services/config.service';
import { PermissionService } from '../../_shared/_services/permission.service';
import { NotificationService } from '../../_shared/_services/notification.service';
import { DeviceService } from '../../_shared/_services/device.service';
import { timer } from 'rxjs';
import { SetpointPopupComponent } from '../../_shared/_components/setpoint-popup/setpoint-popup.component';
import { DialogService } from 'src/app/_shared/_services/dialog.service';
import { EditUtilityDeviceDialogComponent } from '../../_shared/_dialogs/edit-utility-device-dialog/edit-utility-device-dialog.component';
import { EditMeterDeviceDialogComponent } from '../../_shared/_dialogs/edit-meter-device-dialog/edit-meter-device-dialog.component';
import { EditEnergyDeviceDialogComponent } from '../../_shared/_dialogs/edit-energy-device-dialog/edit-energy-device-dialog.component';
import { EditCustomSensorDeviceDialogComponent } from '../../_shared/_dialogs/edit-custom-sensor-device-dialog/edit-custom-sensor-device-dialog.component';
import { EditDistanceDeviceDialogComponent } from '../../_shared/_dialogs/edit-distance-device-dialog/edit-distance-device-dialog.component';
import { EditThermostatClockDeviceDialogComponent } from '../../_shared/_dialogs/edit-thermostat-clock-device-dialog/edit-thermostat-clock-device-dialog.component';
import { ProtectionService } from '../../_shared/_services/protection.service';
import { EditThermostatModeDialogComponent } from '../../_shared/_dialogs/edit-thermostat-mode-dialog/edit-thermostat-mode-dialog.component';
import { EditSetpointDeviceDialogComponent } from '../../_shared/_dialogs/edit-setpoint-device-dialog/edit-setpoint-device-dialog.component';
import { DialogComponent } from '../../_shared/_dialogs/dialog.component';
import { ComponentType } from '@angular/cdk/portal';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: 'dz-utility-widget',
  templateUrl: './utility-widget.component.html',
  styleUrls: ['./utility-widget.component.css']
})
export class UtilityWidgetComponent implements OnInit, OnDestroy {

  @Input() item: Device;

  @Output() private replaceDevice: EventEmitter<void> = new EventEmitter<void>();
  @Output() private removeDevice: EventEmitter<void> = new EventEmitter<void>();

  @ViewChild(SetpointPopupComponent, { static: false }) setpointPopup: SetpointPopupComponent;

  highlight = false;

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    private deviceService: DeviceService,
    private dialogService: DialogService,
    private protectionService: ProtectionService
  ) { }

  ngOnInit() {
    // Trigger highlight effect when creating the component
    // (the component get recreated when RefreshUtilities is called on dashboard page)
    this.highlightItem();
  }

  ngOnDestroy() {
    this.setpointPopup.CloseSetpointPopup();
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

  get bigtext(): string {
    let bigtext = '';
    if ((typeof this.item.Usage !== 'undefined') && (typeof this.item.UsageDeliv === 'undefined')) {
      bigtext += this.item.Usage;
    } else if ((typeof this.item.Usage !== 'undefined') && (typeof this.item.UsageDeliv !== 'undefined')) {
      if ((this.item.UsageDeliv.charAt(0) === '0') || (Number(this.item.Usage) !== 0)) {
        bigtext += this.item.Usage;
      }
      if (this.item.UsageDeliv.charAt(0) !== '0') {
        bigtext += '-' + this.item.UsageDeliv;
      }
    } else if ((this.item.SubType === 'Gas') ||
      (this.item.SubType === 'RFXMeter counter') ||
      (this.item.SubType === 'Counter Incremental')) {
      bigtext += this.item.CounterToday;
    } else if (this.item.SubType === 'Managed Counter') {
      bigtext += this.item.Counter;
    } else if (this.item.Type === 'Air Quality') {
      bigtext += this.item.Data;
    } else if (this.item.SubType === 'Custom Sensor') {
      bigtext += this.item.Data;
    } else if (this.item.Type === 'Current') {
      bigtext += this.item.Data;
    } else if (this.item.SubType === 'Percentage') {
      bigtext += this.item.Data;
    } else if (this.item.SubType === 'Fan') {
      bigtext += this.item.Data;
    } else if (this.item.SubType === 'Soil Moisture') {
      bigtext += this.item.Data;
    } else if (this.item.SubType === 'Leaf Wetness') {
      bigtext += this.item.Data;
    } else if ((this.item.SubType === 'Voltage') ||
      (this.item.SubType === 'Current') ||
      (this.item.SubType === 'Distance') ||
      (this.item.SubType === 'A/D') ||
      (this.item.SubType === 'Pressure') ||
      (this.item.SubType === 'Sound Level')) {
      bigtext += this.item.Data;
    } else if (this.item.Type === 'Lux') {
      bigtext += this.item.Data;
    } else if (this.item.Type === 'Weight') {
      bigtext += this.item.Data;
    } else if (this.item.Type === 'Usage') {
      bigtext += this.item.Data;
    } else if (this.isSetPoint()) {
      bigtext += this.item.Data + '\u00B0 ' + this.configService.config.TempSign;
    } else if (this.item.SubType === 'Waterflow') {
      bigtext += this.item.Data;
    }
    return bigtext;
  }

  get image(): string {
    let img = '';
    if (typeof this.item.Counter !== 'undefined') {
      if ((this.item.Type === 'RFXMeter') ||
        (this.item.Type === 'YouLess Meter') ||
        (this.item.SubType === 'Counter Incremental') ||
        (this.item.SubType === 'Managed Counter')) {
        if (this.item.SwitchTypeVal === 1) {
          img = 'Gas48.png';
        } else if (this.item.SwitchTypeVal === 2) {
          img = 'Water48_On.png';
        } else if (this.item.SwitchTypeVal === 3) {
          img = 'Counter48.png';
        } else if (this.item.SwitchTypeVal === 4) {
          img = 'PV48.png';
        } else {
          img = 'Counter48.png';
        }
      } else {
        if (this.item.SubType === 'Gas') {
          img = 'Gas48.png';
        } else {
          img = 'Counter48.png';
        }
      }
    } else if (this.item.Type === 'Current') {
      img = 'current48.png';
    } else if ((this.item.Type === 'Energy') ||
      (this.item.Type === 'Current/Energy') ||
      (this.item.Type === 'Power') ||
      (this.item.SubType === 'kWh')) {
      if (((this.item.Type === 'Energy') || (this.item.SubType === 'kWh')) && (this.item.SwitchTypeVal === 4)) {
        img = 'PV48.png';
      } else {
        img = 'current48.png';
      }
    } else if (this.item.Type === 'Air Quality') {
      img = 'air48.png';
    } else if (this.item.SubType === 'Custom Sensor') {
      img = '' + this.item.Image + '48_On.png';
    } else if (this.item.SubType === 'Soil Moisture') {
      img = 'moisture48.png';
    } else if (this.item.SubType === 'Percentage') {
      img = 'Percentage48.png';
    } else if (this.item.SubType === 'Fan') {
      img = 'Fan48_On.png';
    } else if (this.item.SubType === 'Leaf Wetness') {
      img = 'leaf48.png';
    } else if (this.item.SubType === 'Distance') {
      img = 'visibility48.png';
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      img = 'current48.png';
    } else if (this.item.SubType === 'Text') {
      img = 'text48.png';
    } else if (this.item.SubType === 'Alert') {
      img = 'Alert48_' + this.item.Level + '.png';
    } else if (this.item.SubType === 'Pressure') {
      img = 'gauge48.png';
    } else if (this.item.Type === 'Lux') {
      img = 'lux48.png';
    } else if (this.item.Type === 'Weight') {
      img = 'scale48.png';
    } else if (this.item.Type === 'Usage') {
      img = 'current48.png';
    } else if (this.isSetPoint()) {
      img = 'override.png';
    } else if (this.item.SubType === 'Thermostat Clock') {
      img = 'clock48.png';
    } else if (this.item.SubType === 'Thermostat Mode') {
      img = 'mode48.png';
    } else if (this.item.SubType === 'Thermostat Fan Mode') {
      img = 'mode48.png';
    } else if (this.item.SubType === 'Sound Level') {
      img = 'Speaker48_On.png';
    } else if (this.item.SubType === 'Waterflow') {
      img = 'moisture48.png';
    }
    return img;
  }

  get status(): string {
    let status = '';

    if (typeof this.item.Counter !== 'undefined') {
      if (
        // (this.item.SubType === 'Gas') ||
        (this.item.SubType === 'RFXMeter counter') ||
        (this.item.SubType === 'Counter Incremental')
      ) {
        status = this.item.Counter;
      } else { // if (this.item.SubType !== 'Managed Counter') {
        status = this.translationService.t('Today') + ': ' + this.item.CounterToday + ', ' + this.item.Counter;
      }
    } else if (this.item.Type === 'Current') {
      status = '';
    } else if ((this.item.Type === 'Energy') ||
      (this.item.Type === 'Current/Energy') ||
      (this.item.Type === 'Power') ||
      (this.item.SubType === 'kWh')) {
      if (typeof this.item.CounterToday !== 'undefined') {
        status += this.translationService.t('Today') + ': ' + this.item.CounterToday;
      }
    } else if (this.item.Type === 'Air Quality') {
      status = this.item.Quality;
    } else if (this.item.SubType === 'Custom Sensor') {
      status = '';
    } else if (this.item.SubType === 'Soil Moisture') {
      status = this.item.Desc;
    } else if (this.item.SubType === 'Percentage') {
      status = '';
    } else if (this.item.SubType === 'Fan') {
      status = '';
    } else if (this.item.SubType === 'Leaf Wetness') {
      status = '';
    } else if (this.item.SubType === 'Distance') {
      status = '';
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      status = '';
    } else if (this.item.SubType === 'Text') {
      status = this.item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
    } else if (this.item.SubType === 'Alert') {
      status = this.item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
    } else if (this.item.SubType === 'Pressure') {
      status = '';
    } else if (this.item.Type === 'Lux') {
      status = '';
    } else if (this.item.Type === 'Weight') {
      status = '';
    } else if (this.item.Type === 'Usage') {
      status = '';
    } else if (this.isSetPoint()) {
      status = '';
    } else if (this.item.SubType === 'Thermostat Clock') {
      status = '';
    } else if (this.item.SubType === 'Thermostat Mode') {
      status = '';
    } else if (this.item.SubType === 'Thermostat Fan Mode') {
      status = '';
    } else if (this.item.SubType === 'Sound Level') {
      status = '';
    } else if (this.item.SubType === 'Waterflow') {
      status = '';
    }

    if (typeof this.item.CounterDeliv !== 'undefined') {
      if (Number(this.item.CounterDeliv) !== 0) {
        status += '<br>' + this.translationService.t('Return') + ': ' + this.translationService.t('Today') + ': ' +
          this.item.CounterDelivToday + ', ' + this.item.CounterDeliv;
      }
    }

    return status;
  }

  isSetPoint(): boolean {
    return ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) || (this.item.Type === 'Radiator 1');
  }

  MakeFavorite(isfavorite: number): void {
    this.deviceService.makeFavorite(Number(this.item.idx), isfavorite).subscribe(_ => {
      this.item.Favorite = isfavorite;
    });
  }

  ShowSetpointPopup(event: any) {
    this.setpointPopup.ShowSetpointPopup(event, this.item.idx, this.item.Protected, this.item.Data);
  }

  onSetpointSet() {
    this.deviceService.getDeviceInfo(this.item.idx).subscribe(device => {
      this.item = device;
    });
  }

  EditUtilityDevice() {
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

    const dialogType: ComponentType<DialogComponent> = this.getEditDialogType();

    const needsProtection: boolean = this.item.SubType === 'Thermostat Clock' ||
      this.item.SubType === 'Thermostat Mode' ||
      this.item.SubType === 'Thermostat Fan Mode' ||
      this.isSetPoint();

    if (needsProtection) {
      this.protectionService.HandleProtection(this.item.Protected, () => {
        this.openDialog(dialogType, dialogOptions);
      });
    } else {
      this.openDialog(dialogType, dialogOptions);
    }

  }

  private openDialog(dialogType: ComponentType<DialogComponent>, dialogOptions: any) {
    const dialog = this.dialogService.addDialog(dialogType, dialogOptions);
    dialog.instance.init();
    dialog.instance.open();
  }

  get logLink(): string[] | null {
    if (typeof this.item.Counter !== 'undefined') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Air Quality') {
      return ['/AirQualityLog', this.item.idx];
    } else if (this.item.SubType === 'Custom Sensor') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Percentage') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Fan') {
      return ['/FanLog', this.item.idx];
    } else if ((this.item.SubType === 'Soil Moisture') || (this.item.SubType === 'Leaf Wetness') || (this.item.SubType === 'Waterflow')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Lux') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Weight') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.Type === 'Usage') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'Energy') || (this.item.SubType === 'kWh') || (this.item.Type === 'Power')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'Current') || (this.item.Type === 'Current/Energy')) {
      return ['/CurrentLog', this.item.idx];
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      // FIXME is it really what we want?
      return this.permissionService.hasPermission('Admin') ? ['/Devices', this.item.idx, 'Log'] : null;
    } else if (this.item.Type === 'Radiator 1') {
      // FIXME is it really what we want?
      return this.permissionService.hasPermission('Admin') ? ['/Devices', this.item.idx, 'Log'] : null;
    } else if (this.item.SubType === 'Text') {
      return ['/Devices', this.item.idx, 'Log'];
    } else if (this.item.SubType === 'Thermostat Clock') {
      return null;
    } else if (this.item.SubType === 'Thermostat Mode') {
      return null;
    } else if (this.item.SubType === 'Thermostat Fan Mode') {
      return null;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Voltage')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Distance')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Current')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Pressure')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Sound Level')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Alert')) {
      return ['/Devices', this.item.idx, 'Log'];
    } else {
      return null;
    }
  }

  // FIXME clean the switches !
  private getEditDialogType(): ComponentType<DialogComponent> {
    if (typeof this.item.Counter !== 'undefined') {
      if (this.item.Type === 'P1 Smart Meter') {
        return EditUtilityDeviceDialogComponent;
      } else {
        return EditMeterDeviceDialogComponent;
      }
    } else if (this.item.Type === 'Air Quality') {
      return EditUtilityDeviceDialogComponent;
    } else if (this.item.SubType === 'Custom Sensor') {
      return EditCustomSensorDeviceDialogComponent;
    } else if (this.item.SubType === 'Percentage') {
      return EditUtilityDeviceDialogComponent;
    } else if (this.item.SubType === 'Fan') {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.SubType === 'Soil Moisture') || (this.item.SubType === 'Leaf Wetness') || (this.item.SubType === 'Waterflow')) {
      return EditUtilityDeviceDialogComponent;
    } else if (this.item.Type === 'Lux') {
      return EditUtilityDeviceDialogComponent;
    } else if (this.item.Type === 'Weight') {
      return EditUtilityDeviceDialogComponent;
    } else if (this.item.Type === 'Usage') {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.Type === 'Energy') || (this.item.SubType === 'kWh') || (this.item.Type === 'Power')) {
      if ((this.item.Type === 'Energy') || (this.item.SubType === 'kWh')) {
        return EditEnergyDeviceDialogComponent;
      } else {
        return EditUtilityDeviceDialogComponent;
      }
    } else if ((this.item.Type === 'Current') || (this.item.Type === 'Current/Energy')) {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.Type === 'Thermostat') && (this.item.SubType === 'SetPoint')) {
      return EditSetpointDeviceDialogComponent;
    } else if (this.item.Type === 'Radiator 1') {
      return EditSetpointDeviceDialogComponent;
    } else if (this.item.SubType === 'Text') {
      return EditUtilityDeviceDialogComponent;
    } else if (this.item.SubType === 'Thermostat Clock') {
      return EditThermostatClockDeviceDialogComponent;
    } else if (this.item.SubType === 'Thermostat Mode') {
      return EditThermostatModeDialogComponent;
    } else if (this.item.SubType === 'Thermostat Fan Mode') {
      return EditThermostatModeDialogComponent;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Voltage')) {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Distance')) {
      return EditDistanceDeviceDialogComponent;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Current')) {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Pressure')) {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.SubType === 'Voltage') || (this.item.SubType === 'Current') || (this.item.SubType === 'A/D')) {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Sound Level')) {
      return EditUtilityDeviceDialogComponent;
    } else if ((this.item.Type === 'General') && (this.item.SubType === 'Alert')) {
      return EditUtilityDeviceDialogComponent;
    } else {
      return EditUtilityDeviceDialogComponent;
    }
  }

}
