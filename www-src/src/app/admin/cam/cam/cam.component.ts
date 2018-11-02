import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {LightSwitchesService} from '../../../_shared/_services/light-switches.service';
import {CamService} from '../../../_shared/_services/cam.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Camera} from 'src/app/_shared/_models/camera';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {DialogService} from '../../../_shared/_services/dialog.service';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {AddEditCameraDialogComponent} from '../../../_shared/_dialogs/add-edit-camera-dialog/add-edit-camera-dialog.component';
import {ConfigService} from '../../../_shared/_services/config.service';
import {DomoticzDevicesService} from '../../../_shared/_services/domoticz-devices.service';

// FIXME proper import
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-cam',
  templateUrl: './cam.component.html',
  styleUrls: ['./cam.component.css']
})
export class CamComponent implements OnInit, AfterViewInit {

  @ViewChild('cameratable', {static: false}) cameratableRef: ElementRef;
  @ViewChild('activetable', {static: false}) activetableRef: ElementRef;

  LightsSwitchesScenes: Array<{ idx: string, name: string, type: number }> = [];

  selectedCamera?: Camera = undefined;
  selectedDevice?: string = undefined;

  activedevice?: string;
  when = '0';
  delay = '0';

  constructor(
    private lightSwitchesService: LightSwitchesService,
    private camService: CamService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private dialogService: DialogService,
    private notificationService: NotificationService,
    private configService: ConfigService,
    private domoticzDevicesService: DomoticzDevicesService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.RefreshLightSwitchesScenesComboArray();
    this.Showcameras();
  }

  Showcameras() {

    $(this.cameratableRef.nativeElement).dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single',
      },
      'aoColumnDefs': [
        {'bSortable': false, 'aTargets': [0, 2, 3]}
      ],
      'aaSorting': [[1, 'asc']],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[25, 50, 100, -1], [25, 50, 100, 'All']],
      'iDisplayLength': 25,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    $(this.activetableRef.nativeElement).dataTable({
      'sDom': '<"H"frC>t<"F"i>',
      'oTableTools': {
        'sRowSelect': 'single',
      },
      'aaSorting': [[0, 'asc']],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    this.RefreshcameraTable();
  }

  private RefreshLightSwitchesScenesComboArray() {
    this.lightSwitchesService.getlightswitchesscenes().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          this.LightsSwitchesScenes.push({
            type: item.type,
            idx: item.idx,
            name: item.Name
          });
        });
      }
    });
  }

  private RefreshcameraTable() {
    $('#updelclr #cameraedit').attr('class', 'btnstyle3-dis');
    $('#updelclr #cameradelete').attr('class', 'btnstyle3-dis');

    const oTable = $(this.cameratableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.camService.getCameras().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let enabledstr = this.translationService.t('No');
          if (item.Enabled === 'true') {
            enabledstr = this.translationService.t('Yes');
          }

          let previewimg;
          const imgsrc = 'camsnapshot.jpg?idx=' + item.idx + '&dtime=' + Math.round(+new Date() / 1000);
          if (item.Enabled === 'true') {
            previewimg = '<img src="' + imgsrc + '" height="40"> ';
          } else {
            previewimg = '<img src="" height="40"> ';
          }

          const captureimg = '<img src="images/capture.png" title="' + this.translationService.t('Take Snapshot') + '" height="32">';
          const captureurl = '<a href="camsnapshot.jpg?idx=' + item.idx + '">' + captureimg + '</a>';

          const streamimg = '<img src="images/webcam.png" title="' + this.translationService.t('Stream Video') + '" height="32">';
          const streamurl = '<a class="js-show-live-stream">' + streamimg + '</a>';

          let protocol;
          if (item.Protocol === 0) {
            protocol = 'HTTP';
          } else {
            protocol = 'HTTPS';
          }

          oTable.fnAddData({
            'DT_RowId': item.idx,
            'Name': item.Name,
            'Address': item.Address,
            'Username': item.Username,
            'Password': item.Password,
            'ImageURL': item.ImageURL,
            'Enabled': item.Enabled,
            'Protocol': item.Protocol,
            'Port': item.Port,
            '0': previewimg,
            '1': item.Name,
            '2': captureurl,
            '3': streamurl,
            '4': enabledstr,
            '5': protocol,
            '6': item.Address,
            '7': item.Port,
            '8': item.ImageURL
          });
        });
      }
    });

    const _this = this;

    oTable.on('click', '.js-show-live-stream', function () {
      const camera = oTable.api().row($(this).closest('tr')).data();

      _this.ShowCameraLiveStream(camera.Name, camera.DT_RowId);
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#cameratable tbody').off();
    $('#cameratable tbody').on('click', 'tr', function () {
      // $.devIdx = -1;

      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#updelclr #cameraedit').attr('class', 'btnstyle3-dis');
        $('#updelclr #cameradelete').attr('class', 'btnstyle3-dis');
        _this.selectedCamera = undefined;
      } else {
        // const oTable = $('#cameratable').dataTable();
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        $('#updelclr #cameraedit').attr('class', 'btnstyle3');
        $('#updelclr #cameradelete').attr('class', 'btnstyle3');
        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const idx = data['DT_RowId'];
          // $.devIdx = idx;
          _this.selectedCamera = data as Camera;
          _this.RefreshActiveDevicesTable(idx);
        }
      }
    });
  }

  private RefreshActiveDevicesTable(idx: string) {
    $('#cameracontent #delclractive #activedevicedelete').attr('class', 'btnstyle3-dis');

    const oTable = $(this.activetableRef.nativeElement).dataTable();
    oTable.fnClearTable();

    this.camService.getActiveDevices(idx).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          let swhen = 'On';
          if (item.when === 1) {
            swhen = 'Off';
          }
          oTable.fnAddData({
            'DT_RowId': item.idx,
            '0': item.Name,
            '1': swhen,
            '2': item.delay
          });
        });
      }
    });

    const _this = this;

    /* Add a click handler to the rows - this could be used as a callback */
    $('#cameracontent #activetable tbody').off();
    $('#cameracontent #activetable tbody').on('click', 'tr', function () {
      if ($(this).hasClass('row_selected')) {
        $(this).removeClass('row_selected');
        $('#cameracontent #delclractive #activedevicedelete').attr('class', 'btnstyle3-dis');
        _this.selectedDevice = undefined;
      } else {
        oTable.$('tr.row_selected').removeClass('row_selected');
        $(this).addClass('row_selected');
        $('#cameracontent #delclractive #activedevicedelete').attr('class', 'btnstyle3');

        const anSelected = DatatableHelper.fnGetSelected(oTable);
        if (anSelected.length !== 0) {
          const data = oTable.fnGetData(anSelected[0]);
          const deviceidx = data['DT_RowId'];
          _this.selectedDevice = deviceidx;
        }
      }
    });
  }

  EditCamera() {
    if (this.selectedCamera) {
      const idx = this.selectedCamera.idx;
      const dialog = this.dialogService.addDialog(AddEditCameraDialogComponent, {
        mode: 'edit', camera: Object.assign({}, this.selectedCamera)
      }).instance;
      dialog.init();
      dialog.open();
      dialog.refresh.subscribe(b => {
        if (b) {
          this.RefreshcameraTable();
        }
      });
    }
  }

  Deletecamera() {
    if (this.selectedCamera) {
      const idx = this.selectedCamera.idx;
      bootbox.confirm(this.translationService.t('Are you sure you want to delete this camera?'), (result) => {
        if (result === true) {
          this.camService.deleteCamera(idx).subscribe(() => {
            this.RefreshcameraTable();
          }, error => {
            this.notificationService.HideNotify();
            this.notificationService.ShowNotify('Problem deleting camera!', 2500, true);
          });
        }
      });
    }
  }

  DeleteActiveDevice() {
    if (this.selectedDevice !== undefined) {
      const idx = this.selectedDevice;
      bootbox.confirm(this.translationService.t('Are you sure to delete this Active Device?\n\nThis action can not be undone...'),
        (result) => {
          if (result === true) {
            this.camService.deleteActiveDevice(idx).subscribe(() => {
              this.RefreshActiveDevicesTable(this.selectedCamera.idx);
            });
          }
        });
    }
  }

  ShowCameraLiveStream(name: string, idx: string) {
    this.domoticzDevicesService.ShowCameraLiveStream(name, idx);
  }

  AddCameraDevice() {
    const dialog = this.dialogService.addDialog(AddEditCameraDialogComponent, {mode: 'add'}).instance;
    dialog.init();
    dialog.open();
    dialog.refresh.subscribe(b => {
      if (b) {
        this.RefreshcameraTable();
      }
    });
  }

  ClearActiveDevices() {
    if (this.selectedCamera) {
      bootbox.confirm(this.translationService.t('Are you sure to delete ALL Active Devices?\n\nThis action can not be undone!!'),
        (result) => {
          if (result === true) {
            this.camService.deleteAllActiveCamDevices(this.selectedCamera.idx).subscribe(() => {
              this.RefreshActiveDevicesTable(this.selectedCamera.idx);
            });
          }
        });
    }
  }

  AddActiveDevice() {
    if (this.selectedCamera === undefined) {
      bootbox.alert('No Camera Selected!');
      return;
    }
    const ActiveDeviceIdx = this.activedevice;
    if (typeof ActiveDeviceIdx === 'undefined') {
      bootbox.alert('No Active Device Selected!');
      return;
    }
    let ADType = 0;
    const ActiveDeviceName = this.LightsSwitchesScenes.find(i => i.idx === ActiveDeviceIdx).name;
    if (ActiveDeviceName.indexOf('[Scene]') === 0) {
      ADType = 1;
    }
    const ADWhen = this.when;
    if (typeof ADWhen === 'undefined') {
      bootbox.alert('Choose When On or Off!');
      return;
    }
    const sdelay = this.delay;
    let ADDelay = 0;
    if (sdelay !== '') {
      ADDelay = parseInt(sdelay);
    }
    this.camService.addCamActiveDevice(this.selectedCamera.idx, ActiveDeviceIdx, ADType, ADWhen, ADDelay).subscribe(data => {
      if (data.status === 'OK') {
        this.RefreshActiveDevicesTable(this.selectedCamera.idx);
      } else {
        this.notificationService.ShowNotify('Problem adding Active Device!', 2500, true);
      }
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify('Problem adding Active Device!', 2500, true);
    });
  }

}
