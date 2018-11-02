import { HardwareService } from '../../../_shared/_services/hardware.service';
import { ActivatedRoute, Router } from '@angular/router';
import { Component, OnInit } from '@angular/core';
import { Hardware } from 'src/app/_shared/_models/hardware';

@Component({
  selector: 'dz-hardware-edit-rfxcom-mode',
  templateUrl: './hardware-edit-rfxcom-mode.component.html',
  styleUrls: ['./hardware-edit-rfxcom-mode.component.css']
})
// tslint:disable:no-bitwise
export class HardwareEditRfxcomModeComponent implements OnInit {

  hardware: Hardware;

  Keeloq: boolean;
  HC: boolean;
  undecon: boolean;
  X10: boolean;
  ARC: boolean;
  AC: boolean;
  HomeEasyEU: boolean;
  Meiantech: boolean;
  OregonScientific: boolean;
  ATIremote: boolean;
  Visonic: boolean;
  Mertik: boolean;
  ADLightwaveRF: boolean;
  HidekiUPM: boolean;
  LaCrosse: boolean;
  Legrand: boolean;
  ProGuard: boolean;
  BlindT0: boolean;
  BlindT1T2T3T4: boolean;
  AEBlyss: boolean;
  Rubicson: boolean;
  FineOffsetViking: boolean;
  Lighting4: boolean;
  RSL: boolean;
  ByronSX: boolean;
  ImaginTronix: boolean;

  asyncType: number;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private router: Router
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      this.Keeloq = (this.hardware.Mode6 & 0x01) !== 0;
      this.HC = (this.hardware.Mode6 & 0x02) !== 0;
      this.undecon = (this.hardware.Mode3 & 0x80) !== 0;
      this.X10 = (this.hardware.Mode5 & 0x01) !== 0;
      this.ARC = (this.hardware.Mode5 & 0x02) !== 0;
      this.AC = (this.hardware.Mode5 & 0x04) !== 0;
      this.HomeEasyEU = (this.hardware.Mode5 & 0x08) !== 0;
      this.Meiantech = (this.hardware.Mode5 & 0x10) !== 0;
      this.OregonScientific = (this.hardware.Mode5 & 0x20) !== 0;
      this.ATIremote = (this.hardware.Mode5 & 0x40) !== 0;
      this.Visonic = (this.hardware.Mode5 & 0x80) !== 0;
      this.Mertik = (this.hardware.Mode4 & 0x01) !== 0;
      this.ADLightwaveRF = (this.hardware.Mode4 & 0x02) !== 0;
      this.HidekiUPM = (this.hardware.Mode4 & 0x04) !== 0;
      this.LaCrosse = (this.hardware.Mode4 & 0x08) !== 0;
      this.Legrand = (this.hardware.Mode4 & 0x10) !== 0;
      this.ProGuard = (this.hardware.Mode4 & 0x20) !== 0;
      this.BlindT0 = (this.hardware.Mode4 & 0x40) !== 0;
      this.BlindT1T2T3T4 = (this.hardware.Mode4 & 0x80) !== 0;
      this.AEBlyss = (this.hardware.Mode3 & 0x01) !== 0;
      this.Rubicson = (this.hardware.Mode3 & 0x02) !== 0;
      this.FineOffsetViking = (this.hardware.Mode3 & 0x04) !== 0;
      this.Lighting4 = (this.hardware.Mode3 & 0x08) !== 0;
      this.RSL = (this.hardware.Mode3 & 0x10) !== 0;
      this.ByronSX = (this.hardware.Mode3 & 0x20) !== 0;
      this.ImaginTronix = (this.hardware.Mode3 & 0x40) !== 0;

      // $('#hardwarecontent #comborego6xxtype').val(Mode1);

      this.asyncType = 0;
      if (this.proXL) {
        this.asyncType = (this.hardware.Extra === '') ? 0 : Number(this.hardware.Extra);
      }
    });
  }

  get proXL(): boolean {
    return this.hardware.version.indexOf('Pro XL') === 0;
  }

  Default() {
    this.asyncType = 0;
    this.Keeloq = false;
    this.HC = false;
    this.undecon = false;
    this.X10 = true;
    this.ARC = true;
    this.AC = true;
    this.HomeEasyEU = true;
    this.Meiantech = false;
    this.OregonScientific = true;
    this.ATIremote = false;
    this.Visonic = false;
    this.Mertik = false;
    this.ADLightwaveRF = false;
    this.HidekiUPM = true;
    this.LaCrosse = true;
    this.Legrand = false;
    this.ProGuard = false;
    this.BlindT0 = false;
    this.BlindT1T2T3T4 = false;
    this.AEBlyss = false;
    this.Rubicson = false;
    this.FineOffsetViking = false;
    this.Lighting4 = false;
    this.RSL = false;
    this.ByronSX = false;
    this.ImaginTronix = false;
  }

  SetRFXCOMMode() {
    const formData = new FormData();
    formData.append('idx', this.hardware.idx as string);
    formData.append('combo_rfx_xl_async_type', this.asyncType.toString());
    if (this.Keeloq) {
      formData.append('Keeloq', 'on');
    }
    if (this.HC) {
      formData.append('HC', 'on');
    }
    if (this.undecon) {
      formData.append('undecon', 'on');
    }
    if (this.X10) {
      formData.append('X10', 'on');
    }
    if (this.ARC) {
      formData.append('ARC', 'on');
    }
    if (this.AC) {
      formData.append('AC', 'on');
    }
    if (this.HomeEasyEU) {
      formData.append('HomeEasyEU', 'on');
    }
    if (this.Meiantech) {
      formData.append('Meiantech', 'on');
    }
    if (this.OregonScientific) {
      formData.append('OregonScientific', 'on');
    }
    if (this.ATIremote) {
      formData.append('ATIremote', 'on');
    }
    if (this.Visonic) {
      formData.append('Visonic', 'on');
    }
    if (this.Mertik) {
      formData.append('Mertik', 'on');
    }
    if (this.ADLightwaveRF) {
      formData.append('ADLightwaveRF', 'on');
    }
    if (this.HidekiUPM) {
      formData.append('HidekiUPM', 'on');
    }
    if (this.LaCrosse) {
      formData.append('LaCrosse', 'on');
    }
    if (this.Legrand) {
      formData.append('Legrand', 'on');
    }
    if (this.ProGuard) {
      formData.append('ProGuard', 'on');
    }
    if (this.BlindT0) {
      formData.append('BlindT0', 'on');
    }
    if (this.BlindT1T2T3T4) {
      formData.append('BlindT1T2T3T4', 'on');
    }
    if (this.AEBlyss) {
      formData.append('AEBlyss', 'on');
    }
    if (this.Rubicson) {
      formData.append('Rubicson', 'on');
    }
    if (this.FineOffsetViking) {
      formData.append('FineOffsetViking', 'on');
    }
    if (this.Lighting4) {
      formData.append('Lighting4', 'on');
    }
    if (this.RSL) {
      formData.append('RSL', 'on');
    }
    if (this.ByronSX) {
      formData.append('ByronSX', 'on');
    }
    if (this.ImaginTronix) {
      formData.append('ImaginTronix', 'on');
    }

    this.hardwareService.setRFXCOMMode(formData).subscribe(() => {
      this.router.navigate(['/dashboard']);
    });
  }

}
