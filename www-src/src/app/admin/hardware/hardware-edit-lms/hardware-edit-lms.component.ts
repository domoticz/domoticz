import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {Hardware} from 'src/app/_shared/_models/hardware';
import {ActivatedRoute, Router} from '@angular/router';
import {HardwareService} from 'src/app/_shared/_services/hardware.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ConfigService} from '../../../_shared/_services/config.service';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-hardware-edit-lms',
  templateUrl: './hardware-edit-lms.component.html',
  styleUrls: ['./hardware-edit-lms.component.css']
})
export class HardwareEditLmsComponent implements OnInit, AfterViewInit {

  hardware: Hardware;

  pollinterval: string;

  @ViewChild('lmsnodestable', { static: false }) lmsnodestable: ElementRef;
  datatable: any;

  constructor(
    private route: ActivatedRoute,
    private hardwareService: HardwareService,
    private router: Router,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    const idx = this.route.snapshot.paramMap.get('idx');
    this.hardwareService.getHardware(idx).subscribe(hardware => {
      this.hardware = hardware;

      this.pollinterval = hardware.Mode1.toString();
    });
  }

  ngAfterViewInit() {
    this.datatable = $(this.lmsnodestable.nativeElement).dataTable({
      sDom: '<"H"lfrC>t<"F"ip>',
      oTableTools: {
        sRowSelect: 'single',
      },
      aaSorting: [[0, 'desc']],
      bSortClasses: false,
      bProcessing: true,
      bStateSave: true,
      bJQueryUI: true,
      aLengthMenu: [[25, 50, 100, -1], [25, 50, 100, 'All']],
      iDisplayLength: 25,
      sPaginationType: 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    this.RefreshLMSNodeTable();
  }

  RefreshLMSNodeTable() {
    // $('#updelclr #nodeupdate').attr('class', 'btnstyle3-dis');
    // $('#updelclr #nodedelete').attr('class', 'btnstyle3-dis');
    // $("#hardwarecontent #lmsnodeparamstable #nodename").val("");
    // $("#hardwarecontent #lmsnodeparamstable #nodeip").val("");
    // $("#hardwarecontent #lmsnodeparamstable #nodeport").val("9000");

    this.datatable.fnClearTable();

    this.hardwareService.getLmsNodes(this.hardware.idx.toString()).subscribe((data) => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          this.datatable.fnAddData({
            'DT_RowId': item.idx,
            'Name': item.Name,
            'Mac': item.Mac,
            'Timeout': item.Status,
            '0': item.idx,
            '1': item.Name,
            '2': item.Mac,
            '3': item.Status
          });
        });
      }
    });
  }

  SetLMSSettings() {
    let Mode1 = Number(this.pollinterval);
    if (Mode1 < 1) {
      Mode1 = 30;
    }
    this.hardwareService.setLmsMode(this.hardware.idx.toString(), Mode1).subscribe(() => {
      bootbox.alert(this.translationService.t('Settings saved'));
    }, error => {
      this.notificationService.ShowNotify(this.translationService.t('Problem Updating Settings!'), 2500, true);
    });
  }

  DeleteUnusedLMSDevices() {
    bootbox.confirm(this.translationService.t('Are you sure to delete all unused devices?'), (result) => {
      if (result === true) {
        this.hardwareService.deleteLmsUnusedDevices(this.hardware.idx.toString()).subscribe(() => {
          this.RefreshLMSNodeTable();
          bootbox.alert(this.translationService.t('Devices deleted'));
        }, error => {
          this.notificationService.ShowNotify(this.translationService.t('Problem Deleting devices!'), 2500, true);
        });
      }
    });
  }

}
