import { ProtectionService } from '../../_services/protection.service';
import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';
import { LightSwitchesService } from 'src/app/_shared/_services/light-switches.service';
import {Device} from "../../_models/device";

@Component({
  selector: 'dz-therm3-popup',
  templateUrl: './therm3-popup.component.html',
  styleUrls: ['./therm3-popup.component.css']
})
export class Therm3PopupComponent implements OnInit {

  @Input() item: Device;

  @Output() switched: EventEmitter<void> = new EventEmitter<void>();

  display = false;

  style: { [key: string]: string; } = {};

  pwd: string;

  constructor(
    private protectionService: ProtectionService,
    private lightSwitchesService: LightSwitchesService
  ) { }

  ngOnInit() {
  }

  public ShowTherm3Popup(_event) {
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

    this.protectionService.HandleProtection(this.item.Protected, (pwd) => {
      this.ShowTherm3PopupInt(mouseX, mouseY, pwd);
    });
  }

  private ShowTherm3PopupInt(mouseX: number, mouseY: number, pwd: string) {
    this.pwd = pwd;
    this.style = {
      width: '176px',
      top: mouseY.toString(),
      left: (mouseX + 15).toString()
    };
    this.display = true;
  }

  SwitchTherm3Popup(cmd: string) {
    this.lightSwitchesService.SwitchLightInt(this.item.idx, cmd, this.pwd).subscribe(() => {
      this.switched.emit();
    });
    this.display = false;
  }

  CloseTherm3Popup() {
    this.display = false;
  }

  ThermUp() {
    this.lightSwitchesService.thermCmd(this.item.idx, 'Up').subscribe(() => {
      // Nothing
    });
  }

  ThermDown() {
    this.lightSwitchesService.thermCmd(this.item.idx, 'Down').subscribe(() => {
      // Nothing
    });
  }

  ThermStop() {
    this.lightSwitchesService.thermCmd(this.item.idx, 'Stop').subscribe(() => {
      // Nothing
    });
  }

  ThermUp2() {
    this.lightSwitchesService.thermCmd(this.item.idx, 'Run Up').subscribe(() => {
      // Nothing
    });
  }

  ThermDown2() {
    this.lightSwitchesService.thermCmd(this.item.idx, 'Run Down').subscribe(() => {
      // Nothing
    });
  }

}
