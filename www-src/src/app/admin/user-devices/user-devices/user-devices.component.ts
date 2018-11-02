import { Component, OnInit, AfterViewInit, Inject } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { UsersService } from 'src/app/_shared/_services/users.service';
import { NotificationService } from '../../../_shared/_services/notification.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { DeviceService } from '../../../_shared/_services/device.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-user-devices',
  templateUrl: './user-devices.component.html',
  styleUrls: ['./user-devices.component.css']
})
export class UserDevicesComponent implements OnInit, AfterViewInit {

  idx: string;
  name: string;
  devices: Array<{ name: string; value: string; }> = [];

  constructor(private route: ActivatedRoute,
    private router: Router,
    private userService: UsersService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private deviceService: DeviceService) { }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');
    this.name = this.route.snapshot.paramMap.get('name');

    this.deviceService.listDevices().subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        this.devices = data.result;
      }
    });
  }

  ngAfterViewInit() {
    const defaultOptions = {
      availableListPosition: 'left',
      splitRatio: 0.5,
      moveEffect: 'blind',
      moveEffectOptions: { direction: 'vertical' },
      moveEffectSpeed: 'fast'
    };

    $('#usercontent .multiselect').multiselect(defaultOptions);

    this.userService.getSharedUserDevices(this.idx).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          // FIXME do this with Angular (if possible)
          const selstr = '#usercontent .multiselect option[value="' + item.DeviceRowIdx + '"]';
          $(selstr).attr('selected', 'selected');
        });
      }
    });
  }

  SaveUserDevices() {
    const selecteddevices: string = $('#usercontent .multiselect option:selected').map(function () { return this.value; }).get().join(';');

    this.userService.setSharedUserDevices(this.idx, selecteddevices).subscribe(() => {
      this.router.navigate(['/Users']);
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem setting User Devices!'), 2500, true);
    });
  }

}
