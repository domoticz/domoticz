import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';
import { ProtectionService } from 'src/app/_shared/_services/protection.service';
import { LightSwitchesService } from 'src/app/_shared/_services/light-switches.service';
import {Device} from "../../_models/device";

@Component({
  selector: 'dz-itho-popup',
  templateUrl: './itho-popup.component.html',
  styleUrls: ['./itho-popup.component.css']
})
export class IthoPopupComponent implements OnInit {

  @Input() item: Device;

  @Output() switched: EventEmitter<void> = new EventEmitter<void>();

  display = false;

  style: { [key: string]: string; } = {};

  constructor(
    private protectionService: ProtectionService,
    private lightSwitchesService: LightSwitchesService
  ) { }

  ngOnInit() {
  }

  public ShowIthoPopup(event) {
    event = event || window.event;
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
      this.ShowIthoPopupInt(mouseX, mouseY);
    });
  }

  private ShowIthoPopupInt(mouseX: number, mouseY: number, ismobile?: boolean) {
    if (typeof ismobile === 'undefined') {
      this.style = {
        'top.px': mouseY.toString(),
        'left.px': (mouseX + 15).toString(),
        'position': 'absolute',
        '-ms-transform': 'none',
        '-moz-transform': 'none',
        '-webkit-transform': 'none',
        'transform': 'none'
      };
    } else {
      this.style = {
        'position': 'fixed',
        'left': '50%',
        'top': '50%',
        '-ms-transform': 'translate(-50%,-50%)',
        '-moz-transform': 'translate(-50%,-50%)',
        '-webkit-transform': 'translate(-50%,-50%)',
        'transform': 'translate(-50%,-50%)'
      };
    }
    this.display = true;
  }

  IthoSendCommand(itho_cmnd: string) {
    const switchcmd = itho_cmnd;
    this.display = false;
    this.lightSwitchesService.SwitchLight(this.item.idx, switchcmd, this.item.Protected).subscribe(() => {
      this.switched.emit();
    });
  }

  CloseIthoPopup() {
    this.display = false;
  }

}
