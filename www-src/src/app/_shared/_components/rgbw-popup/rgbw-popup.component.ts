import { Component, OnInit, Input, Output, EventEmitter, ViewChild, ElementRef } from '@angular/core';
import { ProtectionService } from '../../_services/protection.service';
import { Utils } from 'src/app/_shared/_utils/utils';
import { LightSwitchesService } from '../../_services/light-switches.service';
import { RgbwPickerService } from '../rgbw-picker/rgbw-picker.service';
import {Device} from "../../_models/device";

@Component({
  selector: 'dz-rgbw-popup',
  templateUrl: './rgbw-popup.component.html',
  styleUrls: ['./rgbw-popup.component.css']
})
export class RgbwPopupComponent implements OnInit {

  display = false;

  style: { [key: string]: string; } = {};

  @Input() item: Device;

  @Output() switched: EventEmitter<void> = new EventEmitter<void>();

  @ViewChild('popup', { static: false }) popupRef: ElementRef;

  constructor(
    private protectionService: ProtectionService,
    private lightSwitchesService: LightSwitchesService,
    private rgbwPickerService: RgbwPickerService
  ) { }

  ngOnInit() {
  }

  get ledType() {
    return Utils.getLEDType(this.item.SubType);
  }

  public ShowRGBWPopup(_event) {
    // clearInterval($.setColValue);

    const event = _event || window.event;
    // If pageX/Y aren't available and clientX/Y are,
    // calculate pageX/Y - logic taken from jQuery.
    // (This is to support old IE)
    if (event.pageX == null && event.clientX != null) {
      const eventDoc = (event.target && event.target.ownerDocument) || document;
      const doc = eventDoc.documentElement;
      const body = eventDoc.body;

      event.pageX = event.clientX +
        (doc && doc.scrollLeft || body && body.scrollLeft || 0) -
        (doc && doc.clientLeft || body && body.clientLeft || 0);
      event.pageY = event.clientY +
        (doc && doc.scrollTop || body && body.scrollTop || 0) -
        (doc && doc.clientTop || body && body.clientTop || 0);
    }
    const mouseX = event.pageX;
    const mouseY = event.pageY;

    this.protectionService.HandleProtection(this.item.Protected, () => {
      this.ShowRGBWPopupInt(mouseX, mouseY);
    });
  }

  private ShowRGBWPopupInt(mouseX, mouseY) {
    this.rgbwPickerService.ShowRGBWPicker(
      this.popupRef,
      this.item.idx,
      this.item.Protected ? 1 : 0,
      this.item.MaxDimLevel || 100,
      this.item.LevelInt,
      this.item.Color, // .replace(/\"/g, '\&quot;')
      this.item.SubType,
      this.item.DimmerType
    );

    this.style = {
      'top.px': mouseY,
      'left.px': mouseX + 15
    };
    this.display = true;

    // Update color picker after popup is shown
    this.rgbwPickerService.updateSliders(this.popupRef);
    this.rgbwPickerService.redrawSliders(this.popupRef);
  }

  public SwitchLightPopup(switchcmd: string) {
    this.lightSwitchesService.SwitchLight(this.item.idx, switchcmd, this.item.Protected).subscribe(() => {
      this.switched.emit();
    });
    this.display = false;
  }

  public BrightUp() {
    this.lightSwitchesService.brightnessUp(this.item.idx).subscribe(() => {
      // Nothing
    });
  }

  public BrightDown() {
    this.lightSwitchesService.brightnessDown(this.item.idx).subscribe(() => {
      // Nothing
    });
  }

  public Warmer() {
    this.lightSwitchesService.warmer(this.item.idx).subscribe(() => {
      // Nothing
    });
  }

  public Cooler() {
    this.lightSwitchesService.cooler(this.item.idx).subscribe(() => {
      // Nothing
    });
  }

  public CloseRGBWPopup() {
    this.display = false;
  }

}
