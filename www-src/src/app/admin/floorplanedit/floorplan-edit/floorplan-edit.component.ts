import {AfterViewInit, Component, ElementRef, Inject, OnDestroy, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {FloorplansService} from '../../../_shared/_services/floorplans.service';
import {DatatableHelper} from '../../../_shared/_utils/datatable-helper';
import {ConfigService} from '../../../_shared/_services/config.service';
import {DomoticzDevicesService} from '../../../_shared/_services/domoticz-devices.service';
import {PermissionService} from '../../../_shared/_services/permission.service';
import {catchError, tap} from 'rxjs/operators';
import {forkJoin, interval, Observable, of, Subscription} from 'rxjs';
import {ApiResponse} from '../../../_shared/_models/api';
import {DeviceService} from '../../../_shared/_services/device.service';
import {DialogService} from '../../../_shared/_services/dialog.service';
import {AddEditFloorplanDialogComponent} from '../../../_shared/_dialogs/add-edit-floorplan-dialog/add-edit-floorplan-dialog.component';
import {Utils} from '../../../_shared/_utils/utils';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-floorplan-edit',
  templateUrl: './floorplan-edit.component.html',
  styleUrls: ['./floorplan-edit.component.css']
})
export class FloorplanEditComponent implements OnInit, AfterViewInit, OnDestroy {

  @ViewChild('floorplantable', {static: false}) floorplantableRef: ElementRef;
  @ViewChild('plantable2', {static: false}) plantable2Ref: ElementRef;

  UnusedDevices: Array<{ idx: string, name: string }> = [];

  selectedFloorPlan: FloorplanTableRow = undefined;

  MouseX = 0;
  MouseY = 0;

  LastPlan: string;
  selectedPlan: PlantableTableRow = undefined;

  activeplan: string;

  private mytimer: Subscription;

  constructor(private floorplansService: FloorplansService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private notificationService: NotificationService,
              private configService: ConfigService,
              private permissionService: PermissionService,
              private deviceService: DeviceService,
              private dialogService: DialogService,
              private domoticzDevicesService: DomoticzDevicesService // Necessary even if not used directly
  ) {
  }

  ngOnInit() {
    DomoticzDevicesService.Device.initialise();
  }

  ngAfterViewInit(): void {
    this.ShowFloorPlans();

    const _this = this;
    $('#svgcontainer').resize(function () {
      _this.SVGContainerResize();
    });
  }

  ngOnDestroy(): void {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }

    $('#floorplanimage').off();
    $('#roompolyarea').off();
    $('#svgcontainer').off();
  }

  private ShowFloorPlans() {
    DomoticzDevicesService.Device.useSVGtags = true;
    DomoticzDevicesService.Device.backFunction = () => {
      this.ShowFloorPlans();
    };
    DomoticzDevicesService.Device.switchFunction = () => {
      this.RefreshDevices();
    };
    DomoticzDevicesService.Device.contentTag = 'floorplaneditcontent';

    this.RefreshUnusedDevicesComboArray();

    $(this.floorplantableRef.nativeElement).dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single',
      },
      'bSort': false,
      'bProcessing': true,
      'bStateSave': true,
      'bJQueryUI': true,
      'aLengthMenu': [[10, 25, 100, -1], [10, 25, 100, 'All']],
      'iDisplayLength': 5,
      'sPaginationType': 'full_numbers',
      language: this.configService.dataTableDefaultSettings.language
    });

    $(this.plantable2Ref.nativeElement).dataTable({
      'sDom': '<"H"lfrC>t<"F"ip>',
      'oTableTools': {
        'sRowSelect': 'single',
      },
      'bSort': false,
      'bProcessing': true,
      'bStateSave': false,
      'bJQueryUI': true,
      'aLengthMenu': [[5, 10, 25, 100, -1], [5, 10, 25, 100, 'All']],
      'iDisplayLength': 10,
    });

    this.RefreshFloorPlanTable();
    this.SetButtonStates();
  }

  private RefreshUnusedDevicesComboArray() {
    this.UnusedDevices = [];
    this.floorplansService.getUnusedFloorplanPlans().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          this.UnusedDevices.push({
            idx: item.idx,
            name: item.Name
          });
        });
      }
    });
  }

  private RefreshFloorPlanTable() {
    const _this = this;

    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }

    this.selectedFloorPlan = undefined;

    $('#roomplangroup').empty();
    $('#floorplanimage').attr('xlink:href', '');
    $('#floorplanimagesize').attr('src', '');

    DomoticzDevicesService.Device.initialise();

    const plantable2 = $(this.plantable2Ref.nativeElement).dataTable();
    plantable2.fnClearTable();
    const floorplantable = $(this.floorplantableRef.nativeElement).dataTable();
    floorplantable.fnClearTable();

    this.floorplansService.getFloorplans().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        const totalItems = data.result.length;
        data.result.forEach((item, i) => {
          let updownImg = '';
          if (i !== totalItems - 1) {
            // Add Down Image
            if (updownImg !== '') {
              updownImg += '&nbsp;';
            }
            updownImg += '<img src="images/down.png" class="lcursor js-change-floor-plan-order-1" width="16" height="16"></img>';
          } else {
            updownImg += '<img src="images/empty16.png" width="16" height="16"></img>';
          }
          if (i !== 0) {
            // Add Up image
            if (updownImg !== '') {
              updownImg += '&nbsp;';
            }
            updownImg += '<img src="images/up.png" class="lcursor js-change-floor-plan-order-0" width="16" height="16"></img>';
          }

          const imgsrc = item.Image + '&dtime=' + Math.round(+new Date() / 1000);
          const previewimg = '<img src="' + imgsrc + '" height="40"> ';

          floorplantable.fnAddData(<FloorplanTableRow>{
            'DT_RowId': item.idx,
            'Name': item.Name,
            'Image': item.Image,
            'ScaleFactor': item.ScaleFactor,
            'Order': item.Order,
            'Plans': item.Plans,
            '0': previewimg,
            '1': item.Name,
            '2': (item.Plans > 0) ? item.Plans : '-',
            '3': item.ScaleFactor,
            '4': updownImg
          });
        });

        // handle settings
        if (typeof data.RoomColour !== 'undefined') {
          this.configService.globals.RoomColour = data.RoomColour;
        }
        if (typeof data.ActiveRoomOpacity !== 'undefined') {
          this.configService.globals.ActiveRoomOpacity = data.ActiveRoomOpacity;
          $('.hoverable').css({
            'fill': this.configService.globals.RoomColour,
            'fill-opacity': this.configService.globals.ActiveRoomOpacity / 100,
            'stroke': this.configService.globals.RoomColour,
            'stroke-opacity': 90
          });
        }
        if (typeof data.InactiveRoomOpacity !== 'undefined') {
          this.configService.globals.InactiveRoomOpacity = data.InactiveRoomOpacity;
        }
        if (typeof data.PopupDelay !== 'undefined') {
          DomoticzDevicesService.Device.popupDelay = data.PopupDelay;
        }
        if (typeof data.ShowSensorValues !== 'undefined') {
          DomoticzDevicesService.Device.showSensorValues = (data.ShowSensorValues === 1);
        }
        if (typeof data.ShowSwitchValues !== 'undefined') {
          DomoticzDevicesService.Device.showSwitchValues = (data.ShowSwitchValues === 1);
        }
        if (typeof data.ShowSceneNames !== 'undefined') {
          DomoticzDevicesService.Device.showSceneNames = (data.ShowSceneNames === 1);
        }
      }

      this.SetButtonStates();
    });

    floorplantable.on('click', '.js-change-floor-plan-order-1', function () {
      const row = floorplantable.api().row($(this).closest('tr')).data() as FloorplanTableRow;

      const idx = row.DT_RowId;
      _this.ChangeFloorplanOrder(1, idx);
    });

    floorplantable.on('click', '.js-change-floor-plan-order-0', function () {
      const row = floorplantable.api().row($(this).closest('tr')).data() as FloorplanTableRow;

      const idx = row.DT_RowId;
      _this.ChangeFloorplanOrder(0, idx);
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#floorplantable tbody').off().on('click', 'tr', function () {
      _this.selectedFloorPlan = undefined;
      _this.ConfirmNoUpdate(this, function (param) {
        if ($(param).hasClass('row_selected')) {
          $(param).removeClass('row_selected');
          _this.DeselectFloorplan();
        } else {
          floorplantable.$('tr.row_selected').removeClass('row_selected');
          $(param).addClass('row_selected');
          const anSelected = DatatableHelper.fnGetSelected(floorplantable);
          if (anSelected.length !== 0) {
            const data = floorplantable.fnGetData(anSelected[0]) as FloorplanTableRow;
            const idx = data.DT_RowId;
            _this.selectedFloorPlan = data;
            $('#floorplangroup').attr('scalefactor', data.ScaleFactor);
            _this.RefreshPlanTable(idx);
            $('#floorplanimage').attr('xlink:href', data.Image);
            $('#floorplanimagesize').attr('src', data.Image);
            _this.SVGContainerResize();
          }
        }
        _this.SetButtonStates();
      });
    });
  }

  DeselectFloorplan() {
    $('#floorplangroup').attr('scalefactor', '1.0');
    this.RefreshPlanTable('-1');
    $('#floorplanimage').attr('xlink:href', '');
    $('#floorplanimagesize').attr('src', '');
    if ((typeof $('#floorplaneditor') !== 'undefined') &&
      (typeof $('#floorplanimagesize') !== 'undefined') &&
      (typeof $('#floorplanimagesize')[0] !== 'undefined') &&
      ($('#floorplanimagesize')[0].naturalWidth !== 'undefined')) {
      $('#floorplaneditor')[0].setAttribute('naturalWidth', 1);
      $('#floorplaneditor')[0].setAttribute('naturalHeight', 1);
      $('#floorplaneditor')[0].setAttribute('svgwidth', 1);
      $('#floorplaneditor').height(1);
    }
  }

  private ConfirmNoUpdate(param: any, yesFunc: (any) => void) {
    if ($('#floorplaneditcontent #delclractive #activeplanupdate').attr('class') === 'btnstyle3') {
      bootbox.confirm(this.translationService.t('You have unsaved changes, do you want to continue?'),
        (result: boolean) => {
          if (result === true) {
            yesFunc(param);
          }
        });
    } else {
      yesFunc(param);
    }
  }

  private SetButtonStates() {
    $('#updelclr #floorplanedit').attr('class', 'btnstyle3-dis');
    $('#updelclr #floorplandelete').attr('class', 'btnstyle3-dis');
    $('#floorplaneditcontent #delclractive #activeplanadd').attr('class', 'btnstyle3-dis');
    $('#floorplaneditcontent #delclractive #activeplanclear').attr('class', 'btnstyle3-dis');
    $('#floorplaneditcontent #delclractive #activeplandelete').attr('class', 'btnstyle3-dis');
    $('#floorplaneditcontent #delclractive #activeplanupdate').attr('class', 'btnstyle3-dis');

    let anSelected = DatatableHelper.fnGetSelected($(this.floorplantableRef.nativeElement).dataTable());
    if (anSelected.length !== 0) {
      $('#updelclr #floorplanedit').attr('class', 'btnstyle3');
      $('#updelclr #floorplandelete').attr('class', 'btnstyle3');

      if ($('#floorplaneditcontent #comboactiveplan').children().length > 0) {
        $('#floorplaneditcontent #delclractive #activeplanadd').attr('class', 'btnstyle3');
      }

      anSelected = DatatableHelper.fnGetSelected($(this.plantable2Ref.nativeElement).dataTable());
      if (anSelected.length !== 0) {
        $('#floorplaneditcontent #delclractive #activeplandelete').attr('class', 'btnstyle3');
        const data = $(this.plantable2Ref.nativeElement).dataTable().fnGetData(anSelected[0]) as PlantableTableRow;
        if (data.Area.length !== 0) {
          $('#floorplaneditcontent #delclractive #activeplanclear').attr('class', 'btnstyle3');
        }
      }
    }
  }

  private ChangeFloorplanOrder(order: number, floorplanid: string) {
    if (!this.permissionService.hasPermission('Admin')) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return;
    }
    this.floorplansService.changeFloorplanOrder(floorplanid, order).subscribe(() => {
      this.RefreshFloorPlanTable();
    });
  }

  RefreshDevices() {
    if ((typeof $('#floorplangroup') !== 'undefined') &&
      (typeof $('#floorplangroup')[0] !== 'undefined')) {

      if (typeof this.mytimer !== 'undefined') {
        this.mytimer.unsubscribe();
        this.mytimer = undefined;
      }

      if ($('#floorplangroup')[0].getAttribute('planidx') !== '') {
        DomoticzDevicesService.Device.useSVGtags = true;

        const plan = $('#floorplangroup')[0].getAttribute('planidx');
        this.deviceService.getDevices('all', true, 'Name', this.configService.globals.LastUpdate, plan).subscribe(data => {
          if (typeof data.ActTime !== 'undefined') {
            this.configService.globals.LastUpdate = data.ActTime;
          }

          // insert devices into the document
          if (typeof data.result !== 'undefined') {
            data.result.forEach((_item, i) => {
              const item = {..._item, Scala: $('#floorplangroup')[0].getAttribute('scalefactor')};
              const dev = DomoticzDevicesService.Device.create(item);
              dev.setDraggable($('#floorplangroup')[0].getAttribute('zoomed') === 'false');
              dev.htmlMinimum($('#roomplandevices')[0]);
            });
          }

          // Add Drag and Drop handler
          $('.DeviceIcon').draggable().bind('drag', function (event, ui) {
            // update coordinates manually, since top/left style props don't work on SVG
            const parent = event.target.parentNode;
            if (parent) {
              if (parent.getAttribute('onfloorplan') === 'false') {
                parent.setAttribute('onfloorplan', 'true');
                parent.setAttribute('opacity', '');
              }
              const Scale = DomoticzDevicesService.Device.xImageSize / $('#floorplaneditor').width();
              const offset = $('#floorplanimage').offset();
              let xoffset = Math.round((event.pageX - offset.left - (DomoticzDevicesService.Device.iconSize / 2)) * Scale);
              let yoffset = Math.round((event.pageY - offset.top - (DomoticzDevicesService.Device.iconSize / 2)) * Scale);
              if (xoffset < 0) {
                xoffset = 0;
              }
              if (yoffset < 0) {
                yoffset = 0;
              }
              if (xoffset > (DomoticzDevicesService.Device.xImageSize - DomoticzDevicesService.Device.iconSize)) {
                xoffset = DomoticzDevicesService.Device.xImageSize - DomoticzDevicesService.Device.iconSize;
              }
              if (yoffset > (DomoticzDevicesService.Device.yImageSize - DomoticzDevicesService.Device.iconSize)) {
                yoffset = DomoticzDevicesService.Device.yImageSize - DomoticzDevicesService.Device.iconSize;
              }
              parent.setAttribute('xoffset', xoffset);
              parent.setAttribute('yoffset', yoffset);
              parent.setAttribute('transform',
                'translate(' + xoffset + ',' + yoffset + ') scale(' + $('#floorplangroup').attr('scalefactor') + ')');
              const objData = $('#DeviceDetails #' + event.target.parentNode.id)[0];
              if (objData !== undefined) {
                objData.setAttribute('xoffset', xoffset);
                objData.setAttribute('yoffset', yoffset);
                objData.setAttribute('transform',
                  'translate(' + xoffset + ',' + yoffset + ') scale(' + $('#floorplangroup').attr('scalefactor') + ')');
              }
              $('#floorplaneditcontent #delclractive #activeplanupdate').attr('class', 'btnstyle3');
              if (Utils.matchua(navigator.userAgent).browser === 'mozilla') {
                (event.target.style.display === 'inline') ? event.target.style.display = 'none' : event.target.style.display = 'inline';
              }
            }
          })
            .bind('dragstop', function (event, ui) {
              event.target.style.display = 'inline';
            });

          this.mytimer = interval(10000).subscribe(() => {
            this.RefreshDevices();
          });
        }, error => {
          this.mytimer = interval(10000).subscribe(() => {
            this.RefreshDevices();
          });
        });
      }
    }
  }

  RefreshPlanTable(idx: string) {
    const _this = this;

    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }

    this.LastPlan = idx;

    // if we are zoomed in, zoom out before changing rooms by faking a click in the polygon
    if ($('#floorplangroup')[0].getAttribute('zoomed') === 'true') {
      this.PolyClick();
    }
    $('#roomplangroup').empty();
    $('#roompolyarea').attr('title', '');
    $('#roompolyarea').attr('points', '');
    $('#firstclick').remove();

    const oTable = $(this.plantable2Ref.nativeElement).dataTable();
    oTable.fnClearTable();

    DomoticzDevicesService.Device.initialise();

    this.floorplansService.getFloorplan(idx).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        const totalItems = data.result.length;
        const planGroup = $('#roomplangroup')[0];
        data.result.forEach((item, i) => {
          oTable.fnAddData(<PlantableTableRow>{
            'DT_RowId': item.idx,
            'Area': item.Area,
            '0': item.Name,
            '1': ((item.Area.length === 0) ? '<img src="images/failed.png"/>' : '<img src="images/ok.png"/>'),
            '2': item.Area
          });
          const el = DomoticzDevicesService.makeSVGnode('polygon', {
            id: item.Name + '_Room',
            'class': 'nothoverable',
            points: item.Area
          }, '');
          el.appendChild(DomoticzDevicesService.makeSVGnode('title', null, item.Name));
          planGroup.appendChild(el);
        });
        if ((typeof this.configService.globals.RoomColour !== 'undefined') &&
          (typeof this.configService.globals.InactiveRoomOpacity !== 'undefined')) {
          $('.nothoverable').css({
            'fill': this.configService.globals.RoomColour,
            'fill-opacity': this.configService.globals.InactiveRoomOpacity / 100,
            'stroke': this.configService.globals.RoomColour,
            'stroke-opacity': 90
          });
        }
      }

      this.SetButtonStates();
    });

    /* Add a click handler to the rows - this could be used as a callback */
    $('#plantable2 tbody').off().on('click', 'tr', function () {
      _this.ConfirmNoUpdate(this, function (param) {
        // if we are zoomed in, zoom out before changing rooms by faking a click in the polygon
        if ($('#floorplangroup')[0].getAttribute('zoomed') === 'true') {
          _this.PolyClick();
        }
        if ($(param).hasClass('row_selected')) {
          $(param).removeClass('row_selected');
          DomoticzDevicesService.Device.initialise();
          $('#roompolyarea').attr('title', '');
          $('#roompolyarea').attr('points', '');
          $('#firstclick').remove();
          $('#floorplangroup').attr('planidx', '');
          $('#floorplanimage').css('cursor', 'auto');
        } else {
          oTable.$('tr.row_selected').removeClass('row_selected');
          $(param).addClass('row_selected');
          DomoticzDevicesService.Device.initialise();
          const anSelected = DatatableHelper.fnGetSelected(oTable);
          if (anSelected.length !== 0) {
            const data = oTable.fnGetData(anSelected[0]) as PlantableTableRow;
            const planidx = data['DT_RowId'];
            _this.selectedPlan = data;
            $('#roompolyarea').attr('title', data['0']);
            $('#roompolyarea').attr('points', data['2']);
            $('#floorplangroup').attr('planidx', planidx);
            $('#floorplanimage').css('cursor', 'crosshair');
            _this.ShowDevices(planidx);
          }
        }
        _this.SetButtonStates();
      });
    });
  }

  ShowDevices(idx: string) {
    if (typeof $('#floorplangroup') !== 'undefined') {
      DomoticzDevicesService.Device.useSVGtags = true;
      DomoticzDevicesService.Device.initialise();
      $('#roomplandevices').empty();
      $('#floorplangroup')[0].setAttribute('planidx', idx);
      this.configService.globals.LastUpdate = 0;
      this.RefreshDevices();
    }
  }

  EditFloorplan() {
    const idx = this.devIdx;

    const dialogRef = this.dialogService.addDialog(AddEditFloorplanDialogComponent, {
      floorplanname: this.selectedFloorPlan.Name,
      scalefactor: this.selectedFloorPlan.ScaleFactor,
      editMode: true,
      callbackFn: (csettings) => {
        this.UpdateFloorplan(idx, csettings);
      }
    });
    dialogRef.instance.init();
    dialogRef.instance.open();
  }

  UpdateFloorplan(idx: string, csettings?: { name: string, scalefactor: string }) {
    if (typeof csettings === 'undefined') {
      return;
    }

    this.floorplansService.updateFloorplan(idx, csettings.name, csettings.scalefactor).subscribe(() => {
      this.RefreshFloorPlanTable();
    }, error => {
      this.notificationService.ShowNotify('Problem updating Plan settings!', 2500, true);
    });
  }

  DeleteFloorplan() {
    const idx = this.devIdx;

    bootbox.confirm(this.translationService.t('Are you sure you want to delete this Floorplan?'), (result: boolean) => {
      if (result === true) {
        this.floorplansService.deleteFloorplan(idx).subscribe(() => {
          this.RefreshUnusedDevicesComboArray();
          this.RefreshFloorPlanTable();
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify('Problem deleting Floorplan!', 2500, true);
          this.RefreshUnusedDevicesComboArray();
          this.RefreshFloorPlanTable();
        });
      }
    });
  }

  DeleteFloorplanPlan() {
    const planidx = this.selectedPlan.DT_RowId;

    bootbox.confirm(this.translationService.t('Are you sure to delete this Plan from the Floorplan?'), (result) => {

      if (result === true) {
        this.floorplansService.deleteFloorplanPlan(planidx).subscribe(() => {
          this.RefreshUnusedDevicesComboArray();
          this.RefreshPlanTable(this.devIdx);
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify('Problem deleting Plan!', 2500, true);
        });
      }
    });
  }

  UpdateFloorplanPlan(clear: boolean) {
    const planidx = this.selectedPlan.DT_RowId;

    let PlanArea = '';
    if (clear !== true) {
      PlanArea = $('#roompolyarea').attr('points');
    }

    this.floorplansService.updateFloorplanPlan(planidx, PlanArea).pipe(
      tap(data => {
        if (data.status === 'OK') {
          // RefreshPlanTable($.devIdx);
        } else {
          this.notificationService.ShowNotify('Problem updating Plan!', 2500, true);
        }
      }),
      catchError(error => {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify('Problem updating Plan!', 2500, true);
        return of(undefined);
      })
    ).subscribe(() => {
      const deviceParent = $('#DeviceIcons')[0];
      const calls: Array<Observable<ApiResponse>> = (Array.from(deviceParent.childNodes) as Array<HTMLElement>)
        .filter(dev => {
          return dev.getAttribute('onfloorplan') !== 'false';
        })
        .map(dev => {
          return this.floorplansService.setPlanDeviceCoords(
            dev.getAttribute('idx'),
            planidx,
            dev.getAttribute('xoffset'),
            dev.getAttribute('yoffset'),
            dev.getAttribute('devscenetype')
          ).pipe(
            tap(data => {
              if (data.status === 'OK') {
                // 						RefreshPlanTable($.devIdx);
              } else {
                this.notificationService.ShowNotify('Problem udating Device Coordinates!', 2500, true);
              }
            }),
            catchError(error => {
              this.notificationService.HideNotify();
              this.notificationService.ShowNotify('Problem updating Device Coordinates!', 2500, true);
              return of(undefined);
            })
          );
        });
      forkJoin(calls).subscribe(() => {
        this.RefreshPlanTable(this.devIdx);
      });
    });
  }

  AddNewFloorplan() {
    this.DeselectFloorplan();

    const dialogRef = this.dialogService.addDialog(AddEditFloorplanDialogComponent, {
      floorplanname: '',
      scalefactor: '1.0',
      imagefile: '',
      editMode: false,
      callbackFn: (csettings) => {
        this.AddFloorplan(csettings);
      }
    });
    dialogRef.instance.init();
    dialogRef.instance.open();
  }

  AddFloorplan(csettings?: { name: string, scalefactor: string; file: File }) {
    if (typeof csettings === 'undefined') {
      return;
    }

    this.floorplansService.uploadFloorplanImage(csettings.name, csettings.scalefactor, csettings.file).subscribe(() => {
      this.RefreshFloorPlanTable();
    }, error => {
      this.notificationService.ShowNotify('Problem adding Floorplan!', 2500, true);
    });
  }

  AddUnusedPlan() {
    if (!this.selectedFloorPlan) {
      return;
    }

    const PlanIdx = this.activeplan;
    if (typeof PlanIdx === 'undefined') {
      return;
    }

    this.floorplansService.addFloorplan(this.devIdx, PlanIdx).subscribe(data => {
      if (data.status === 'OK') {
        this.RefreshPlanTable(this.devIdx);
        this.RefreshUnusedDevicesComboArray();
      } else {
        this.notificationService.ShowNotify('Problem adding Plan!', 2500, true);
      }

      this.SetButtonStates();
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify('Problem adding Plan!', 2500, true);

      this.SetButtonStates();
    });
  }

  FloorplanClick(event: any) {
    // make sure we aren't zoomed in.
    if ($('#floorplangroup')[0].getAttribute('zoomed') !== 'true') {
      if ($('#roompolyarea').attr('title') !== '') {
        const Scale = DomoticzDevicesService.Device.xImageSize / $('#floorplaneditor').width();
        const offset = $('#floorplanimage').offset();
        let points = $('#roompolyarea').attr('points');
        const xPoint = Math.round((event.pageX - offset.left) * Scale);
        const yPoint = Math.round((event.pageY - offset.top) * Scale);
        if (points !== '') {
          points = points + ',';
        } else {
          $('#floorplangroup')[0].appendChild(DomoticzDevicesService.makeSVGnode('circle', {
            id: 'firstclick',
            cx: xPoint,
            cy: yPoint,
            r: 2,
            class: 'hoverable'
          }, ''));
          $('.hoverable').css({
            'fill': this.configService.globals.RoomColour,
            'fill-opacity': this.configService.globals.ActiveRoomOpacity / 100,
            'stroke': this.configService.globals.RoomColour,
            'stroke-opacity': 90
          });
        }
        points = points + xPoint + ',' + yPoint;
        $('#roompolyarea').attr('points', points);
        $('#floorplaneditcontent #delclractive #activeplanupdate').attr('class', 'btnstyle3');
      } else {
        this.notificationService.ShowNotify('Select a Floorplan and Room first.', 2500, true);
      }
    } else {
      this.PolyClick(event);
    }
  }

  ImageLoaded() {
    if ((typeof $('#floorplanimagesize') !== 'undefined') &&
      (typeof $('#floorplanimagesize')[0] !== 'undefined') &&
      ($('#floorplanimagesize')[0].naturalWidth !== 'undefined')) {
      $('#helptext').attr('title',
        'Image width is: ' + $('#floorplanimagesize')[0].naturalWidth + ', Height is: ' + $('#floorplanimagesize')[0].naturalHeight);
      $('#svgcontainer')[0].setAttribute('viewBox',
        '0 0 ' + $('#floorplanimagesize')[0].naturalWidth + ' ' + $('#floorplanimagesize')[0].naturalHeight);
      DomoticzDevicesService.Device.xImageSize = $('#floorplanimagesize')[0].naturalWidth;
      DomoticzDevicesService.Device.yImageSize = $('#floorplanimagesize')[0].naturalHeight;
      $('#svgcontainer').show();
      this.SVGContainerResize();
    }
  }

  SVGContainerResize() {
    if ((typeof $('#floorplaneditor') !== 'undefined') &&
      (typeof $('#floorplanimagesize') !== 'undefined') &&
      (typeof $('#floorplanimagesize')[0] !== 'undefined') &&
      ($('#floorplanimagesize')[0].naturalWidth !== 'undefined')) {
      $('#floorplaneditor')[0].setAttribute('naturalWidth', $('#floorplanimagesize')[0].naturalWidth);
      $('#floorplaneditor')[0].setAttribute('naturalHeight', $('#floorplanimagesize')[0].naturalHeight);
      $('#floorplaneditor')[0].setAttribute('svgwidth', $('#svgcontainer').width());
      const ratio = $('#floorplanimagesize')[0].naturalWidth / $('#floorplanimagesize')[0].naturalHeight;
      $('#floorplaneditor')[0].setAttribute('ratio', ratio);
      const svgHeight = $('#floorplaneditor').width() / ratio;
      $('#floorplaneditor').height(svgHeight);
    }
  }

  SVGContainerMouseMove(event: any) {
    const Scale = DomoticzDevicesService.Device.xImageSize / $('#floorplaneditor').width();
    const offset = $('#floorplanimage').offset();
    const xoffset = Math.round((event.pageX - offset.left) * Scale);
    const yoffset = Math.round((event.pageY - offset.top) * Scale);

    if (xoffset < 0) {
      return;
    }
    if (yoffset < 0) {
      return;
    }

    this.MouseX = xoffset;
    this.MouseY = yoffset;
    if ($('#floorplangroup').attr('zoomed') === 'false') {
      if ($('#guidelines').children().length === 0) {
        $('#guidelines').append(DomoticzDevicesService.makeSVGnode('line', {
          id: 'vertLine',
          x1: xoffset,
          y1: '0',
          x2: xoffset,
          y2: 5000,
          style: 'cursor:crosshair; stroke:rgb(255,0,0);stroke-width:2'
        }, '', ''));
        $('#guidelines').append(DomoticzDevicesService.makeSVGnode('line', {
          id: 'horiLine',
          x1: '0',
          y1: yoffset,
          x2: 5000,
          y2: yoffset,
          style: 'cursor:crosshair; stroke:rgb(255,0,0);stroke-width:2'
        }, '', ''));
      } else {
        $('#vertLine').attr('x1', xoffset);
        $('#vertLine').attr('x2', xoffset);
        $('#horiLine').attr('y1', yoffset);
        $('#horiLine').attr('y2', yoffset);
      }
    } else {
      $('#guidelines').empty();
    }
  }

  SVGContainerMouseLeave() {
    $('#guidelines').empty();
  }

  PolyClick(event?: any) {
    if ($('#floorplangroup')[0].getAttribute('zoomed') === 'true') {
      $('#floorplangroup')[0].setAttribute('transform', 'translate(0,0) scale(1)');
      $('#DeviceContainer')[0].setAttribute('transform', 'translate(0,0) scale(1)');
      $('#floorplangroup')[0].setAttribute('zoomed', 'false');
    } else {
      const borderRect = event.target.getBBox(); // polygon bounding box
      const margin = 0.1;  // 10% margin around polygon
      const marginX = borderRect.width * margin;
      const marginY = borderRect.height * margin;
      const scaleX = DomoticzDevicesService.Device.xImageSize / (borderRect.width + (marginX * 2));
      const scaleY = DomoticzDevicesService.Device.yImageSize / (borderRect.height + (marginY * 2));
      const scale = ((scaleX > scaleY) ? scaleY : scaleX);
      const attr = 'scale(' + scale + ')' + ' translate(' + (borderRect.x - marginX) * -1 + ',' + (borderRect.y - marginY) * -1 + ')';

      // this actually needs to centre in the direction its not scaling but close enough for v1.0
      $('#floorplangroup')[0].setAttribute('transform', attr);
      $('#DeviceContainer')[0].setAttribute('transform', attr);
      $('#floorplangroup')[0].setAttribute('zoomed', 'true');
    }
    this.configService.globals.LastUpdate = 0;
    this.RefreshDevices(); // force redraw to change 'moveability' of icons
  }

  get devIdx(): string | undefined {
    return this.selectedFloorPlan ? this.selectedFloorPlan.DT_RowId : undefined;
  }

}

interface FloorplanTableRow {
  'DT_RowId': string;
  'Name': string;
  'Image': string;
  'ScaleFactor': string;
  'Order': string;
  'Plans': number;
  '0': string;
  '1': string;
  '2': string;
  '3': string;
  '4': string;
}

interface PlantableTableRow {
  'DT_RowId': string;
  'Area': string;
  '0': string;
  '1': string;
  '2': string;
}
