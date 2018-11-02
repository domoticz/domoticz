import { Injectable, Inject } from '@angular/core';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { NotificationService } from './notification.service';
import { DeviceService } from './device.service';
import { Utils } from '../_utils/utils';
import { timer, Subscription, interval } from 'rxjs';
import { ApiService } from './api.service';
import { PermissionService } from './permission.service';
import { ConfigService } from './config.service';
import { LightSwitchesService } from './light-switches.service';
import { Router } from '@angular/router';
import { SceneService } from './scene.service';
import { DialogService } from './dialog.service';
import { ComponentType } from '@angular/cdk/portal';
import { DialogComponent } from '../_dialogs/dialog.component';
import { MediaRemoteDialogComponent } from '../_dialogs/media-remote-dialog/media-remote-dialog.component';
import { LmsPlayerRemoteDialogComponent } from '../_dialogs/lms-player-remote-dialog/lms-player-remote-dialog.component';
import { HeosPlayerRemoteDialogComponent } from '../_dialogs/heos-player-remote-dialog/heos-player-remote-dialog.component';
import { CameraLiveDialogComponent } from '../_dialogs/camera-live-dialog/camera-live-dialog.component';

// FIXME proper declaration
declare var $: any;

// FIXME this service is a mess (especially the inner classes), clean it!
@Injectable()
export class DomoticzDevicesService {

  public static Device: DomoticzDevice;

  constructor(
    @Inject(I18NEXT_SERVICE) public translationService: ITranslationService,
    public notificationService: NotificationService,
    public deviceService: DeviceService,
    public apiService: ApiService,
    public permissionService: PermissionService,
    public configService: ConfigService,
    public lightSwitchesService: LightSwitchesService,
    public router: Router,
    public sceneService: SceneService,
    public dialogService: DialogService
  ) {
    DomoticzDevicesService.Device = new DomoticzDevice(this);
  }

  public static makeSVGnode(tag: string, attrs: any, text: string, title?: string): SVGElement {
    const el = document.createElementNS('http://www.w3.org/2000/svg', tag);
    for (const k in attrs) {
      if (k === 'xlink:href') {
        el.setAttributeNS('http://www.w3.org/1999/xlink', 'href', attrs[k]);
      } else {
        el.setAttribute(k, attrs[k]);
      }
    }
    if ((typeof text !== 'undefined') && (text.length !== 0)) {
      el.appendChild(document.createTextNode(text));
    }
    if ((typeof title !== 'undefined') && (title.length !== 0)) {
      const elTitle = document.createElementNS('http://www.w3.org/2000/svg', 'title');
      if (title.length !== 0) {
        elTitle.appendChild(document.createTextNode(title));
      }
      el.appendChild(elTitle);
    }
    return el;
  }

  public static getSVGnode() {
    const divFloors = $('.imageparent');
    if (divFloors.length === 0) {
      return;
    }
    let i = 0;
    while (i < divFloors[0].children.length) {
      if (divFloors[0].children[i].tagName === 'svg') {
        return divFloors[0].children[i];
      }
      i++;
    }
    return;
  }

  public static makeSVGmultiline(attrs: any, text: string, title: string,
    maxX: number, minY: number, incY: number, separator?: string): SVGElement {
    // no wrap parameters
    if ((typeof maxX === 'undefined') || (typeof minY === 'undefined') || (typeof incY === 'undefined')) {
      return DomoticzDevicesService.makeSVGnode('text', attrs, text, title);
    }
    // no text to wrap
    if ((typeof text === 'undefined') || (text.length === 0)) {
      return DomoticzDevicesService.makeSVGnode('text', attrs, text, title);
    }

    const elText = DomoticzDevicesService.makeSVGnode('text', attrs, '', title);
    const svg = DomoticzDevicesService.getSVGnode();
    svg.appendChild(elText);
    let elSpan: any = DomoticzDevicesService.makeSVGnode('tspan', {}, text);
    elText.appendChild(elSpan);
    if (elSpan.getComputedTextLength() <= maxX) {
      svg.removeChild(svg.childNodes[svg.childNodes.length - 1]);
      return elText;
    }
    // now things get interesting
    elText.setAttribute('y', minY.toString());
    elSpan.firstChild.data = '';

    if (typeof separator === 'undefined') {
      separator = ', ';
    }
    let phrase = text.split(separator);
    if (phrase === [text]) {
      separator = ' ';
      phrase = text.split(separator);
    }
    let len = 0;
    for (let i = 0; i < phrase.length; i++) {
      if (separator === '<br />') {
        elSpan = DomoticzDevicesService.makeSVGnode('tspan', {}, text);
        elText.appendChild(elSpan);
        elSpan.setAttribute('x', 0);
        elSpan.setAttribute('dy', incY);
        elSpan.firstChild.data = phrase[i];
      } else {
        if (i !== 0) {
          elSpan.firstChild.data += separator;
        }
        len = elSpan.firstChild.data.length;
        elSpan.firstChild.data += phrase[i];

        if (elSpan.getComputedTextLength() > maxX) {
          elSpan.firstChild.data = elSpan.firstChild.data.slice(0, len).trim();    // Remove added word
          elSpan = DomoticzDevicesService.makeSVGnode('tspan', {}, text);
          elText.appendChild(elSpan);
          elSpan.setAttribute('x', 0);
          elSpan.setAttribute('dy', incY);
          elSpan.firstChild.data = phrase[i];
        }
      }
    }
    svg.removeChild(svg.childNodes[svg.childNodes.length - 1]);
    return elText;
  }

  public static getMaxSpanWidth(elText): number {
    let iLength = 0;
    let iMaxSpan = 0;

    const svg = DomoticzDevicesService.getSVGnode();
    svg.appendChild(elText);
    for (let i = 0; i < elText.childNodes.length; i++) {
      iLength = elText.childNodes[i].getComputedTextLength();
      iMaxSpan = (iLength > iMaxSpan) ? iLength : iMaxSpan;
    }
    svg.removeChild(svg.lastChild);

    return iMaxSpan;
  }

  public static SVGTextLength(attrs: any, text: string): number {
    let iLength = 0;
    // no text to test
    if ((typeof text !== 'undefined') && (text.length > 0)) {
      const svg = DomoticzDevicesService.getSVGnode();
      const elText = DomoticzDevicesService.makeSVGnode('text', attrs, '', '');
      svg.appendChild(elText);
      const elSpan: any = DomoticzDevicesService.makeSVGnode('tspan', {}, text);
      elText.appendChild(elSpan);
      iLength = elSpan.getComputedTextLength();
      svg.removeChild(svg.lastChild);
    }

    return iLength;
  }

  public ShowMediaRemote(name: string, index: number, HardwareType: string) {
    let dialogComponent: ComponentType<DialogComponent>;
    if (HardwareType.indexOf('Kodi') >= 0 || HardwareType.indexOf('Panasonic') >= 0) {
      dialogComponent = MediaRemoteDialogComponent;
    } else if (HardwareType.indexOf('Logitech Media Server') >= 0) {
      dialogComponent = LmsPlayerRemoteDialogComponent;
    } else if (HardwareType.indexOf('HEOS by DENON') >= 0) {
      dialogComponent = HeosPlayerRemoteDialogComponent;
    } else {
      return;
    }
    const dialog = this.dialogService.addDialog(dialogComponent, {
      Name: name,
      devIdx: index,
      HWType: HardwareType
    }).instance;
    dialog.init();
    dialog.open();
  }

  ShowCameraLiveStream(name: string, cameraIdx: string) {
    const dialog = this.dialogService.addDialog(CameraLiveDialogComponent, { item: { Name: name, CameraIdx: cameraIdx } }).instance;
    dialog.init();
    dialog.open();
  }

}

export class DomoticzDevice {

  count = 0;
  notPositioned = 0;
  useSVGtags = false;
  contentTag = '';
  xImageSize = 1280;
  yImageSize = 720;
  iconSize = 32;
  elementPadding = 5;
  showSensorValues = true;
  showSceneNames = false;
  showSwitchValues = false;
  ignoreClick = false;
  popupDelay = 0;
  popupDelayDevice = '';
  popupDelayTimer: number | Subscription = 0;
  expandTarget = null;
  expandRect = null;
  expandShadow = null;
  expandVar: Subscription = null;
  expandRot = null;
  expandInt = null;
  expandInc = null;
  expandEnd = null;
  index: any;
  floorID: number;
  planID: number;
  xoffset: number;
  yoffset: number;
  scale: number;
  width: number;
  height: number;
  type: any;
  devSceneType: number;
  subtype: any;
  status: any;
  lastupdate: any;
  protected: any;
  name: any;
  imagetext: string;
  image: string;
  image_opacity: number;
  image2: string;
  image2_opacity: number;
  favorite: any;
  forecastURL: any;
  batteryLevel: any;
  haveTimeout: any;
  haveCamera: any;
  cameraIdx: any;
  haveDimmer: boolean;
  level: number;
  levelMax: number;
  data: any;
  hasNewLine: boolean;
  smallStatus: any;
  sensorunit: any;
  uniquename: string;
  controlable: boolean;
  switchType: any;
  switchTypeVal: any;
  onClick: () => void;
  onClick2: () => void;
  LogLink: string[];
  EditLink: string[];
  TimerLink: string[];
  NotifyLink: string[];
  WebcamClick: () => void;
  hasNotifications: boolean;
  moveable: boolean;
  onFloorplan: boolean;
  showStatus: boolean;
  backFunction: () => void = () => { };
  switchFunction: () => void = () => { };

  constructor(protected thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      this.index = item.idx;
      this.floorID = (typeof item.FloorID !== 'undefined') ? Number(item.FloorID) : 0;
      this.planID = (typeof item.PlanID !== 'undefined') ? Number(item.PlanID) : 0;
      this.xoffset = (typeof item.XOffset !== 'undefined') ? Number(item.XOffset) : 0;
      this.yoffset = (typeof item.YOffset !== 'undefined') ? Number(item.YOffset) : 0;
      this.scale = (typeof item.Scale !== 'undefined') ? parseFloat(item.Scale) : 1.0;
      this.width = DomoticzDevicesService.Device.elementPadding * 50;
      this.height = DomoticzDevicesService.Device.elementPadding * 15;
      this.type = item.Type;
      this.devSceneType = ((item.Type === 'Scene') || (item.Type === 'Group')) ? 1 : 0;
      this.subtype = item.SubType;
      this.status = (typeof item.Status === 'undefined') ? '' : item.Status;
      this.lastupdate = item.LastUpdate;
      this.protected = item.Protected;
      this.name = item.Name;
      this.imagetext = '';
      this.image = 'images/unknown.png';
      this.image_opacity = 1;
      this.image2 = '';
      this.image2_opacity = 1;
      this.favorite = item.Favorite;
      this.forecastURL = (typeof item.forecast_url === 'undefined') ? '' : item.forecast_url;
      this.batteryLevel = (typeof item.BatteryLevel !== 'undefined') ? item.BatteryLevel : 0;
      this.haveTimeout = (typeof item.HaveTimeout !== 'undefined') ? item.HaveTimeout : false;
      this.haveCamera = ((typeof item.UsedByCamera !== 'undefined') && (typeof item.CameraIdx !== 'undefined'))
        ? item.UsedByCamera : false;
      this.cameraIdx = (typeof item.CameraIdx !== 'undefined') ? item.CameraIdx : 0;
      this.haveDimmer = false;  // data from server is unreliable so inheriting classes will need to force true
      this.level = (typeof item.LevelInt !== 'undefined') ? Number(item.LevelInt) : 0;
      this.levelMax = (typeof item.MaxDimLevel !== 'undefined') ? Number(item.MaxDimLevel) : 0;
      if (this.level > this.levelMax) {
        this.levelMax = this.level;
      }

      this.data = (typeof item.Data !== 'undefined') ? item.Data : '';
      this.hasNewLine = false;

      this.smallStatus = this.data;
      if (typeof item.Usage !== 'undefined') {
        this.data = item.Usage;
      }
      if (typeof item.SensorUnit !== 'undefined') {
        this.sensorunit = item.SensorUnit;
      }
      this.uniquename = item.Type.replace(/\s+/g, '') + '_' + this.floorID + '_' + this.planID + '_' + this.index;
      this.controlable = false;
      this.switchType = (typeof item.SwitchType !== 'undefined') ? item.SwitchType : '';
      this.switchTypeVal = (typeof item.SwitchTypeVal !== 'undefined') ? item.SwitchTypeVal : '';
      this.onClick = null;
      this.onClick2 = null;
      this.LogLink = [];
      this.EditLink = [];
      this.TimerLink = [];
      this.NotifyLink = [];
      this.WebcamClick = null;
      this.hasNotifications = (item.Notifications === 'true') ? true : false;
      this.moveable = false;
      this.onFloorplan = true;
      this.showStatus = false;

      // store in page for later use
      if ($('#DeviceData')[0] !== undefined && $('#DeviceData')[0] !== null) {
        const el = DomoticzDevicesService.makeSVGnode('data', { id: this.uniquename + '_Data', item: JSON.stringify(item) }, '');
        const existing = document.getElementById(this.uniquename + '_Data');
        if (existing != null) {
          $('#DeviceData')[0].replaceChild(el, existing);
        } else {
          $('#DeviceData')[0].appendChild(el);
        }
      }

      DomoticzDevicesService.Device.count++;
      if ((this.xoffset === 0) && (this.yoffset === 0)) {
        const iconSpacing = 75;
        const iconsPerColumn = Math.floor(DomoticzDevicesService.Device.yImageSize / iconSpacing);
        this.yoffset += DomoticzDevicesService.Device.notPositioned * iconSpacing;
        while ((this.yoffset + 50) > DomoticzDevicesService.Device.yImageSize) {
          this.xoffset += iconSpacing;
          this.yoffset -= (iconsPerColumn * iconSpacing);
        }
        DomoticzDevicesService.Device.notPositioned++;
        this.onFloorplan = false;
      }
    }
  }

  checkDefs() {
    if (DomoticzDevicesService.Device.useSVGtags === true) {
      const divFloors = $('.imageparent');
      if (divFloors.length === 0) {
        return;
      }
      const cont = divFloors[0].children[0];
      if ($('#DeviceData')[0] === undefined || $('#DeviceData')[0] === null) {
        cont.appendChild(DomoticzDevicesService.makeSVGnode('g', { id: 'DeviceData' }, ''));
      }
      if ($('#DeviceDefs')[0] === undefined || $('#DeviceDefs')[0] === null) {
        if (cont !== undefined && cont !== null) {
          const defs = DomoticzDevicesService.makeSVGnode('defs', { id: 'DeviceDefs' }, '');
          cont.appendChild(defs);
          let linGrad = DomoticzDevicesService.makeSVGnode('linearGradient', {
            id: 'PopupGradient',
            x1: '0%',
            y1: '0%',
            x2: '100%',
            y2: '100%'
          }, '');
          defs.appendChild(linGrad);
          linGrad.appendChild(DomoticzDevicesService.makeSVGnode('stop', {
            offset: '25%',
            style: 'stop-color:rgb(255,255,255);stop-opacity:1'
          }, ''));
          linGrad.appendChild(DomoticzDevicesService.makeSVGnode('stop',
            {
              offset: '100%',
              style: 'stop-color:rgb(225,225,225);stop-opacity:1'
            },
            ''));
          linGrad = DomoticzDevicesService.makeSVGnode('linearGradient',
            {
              id: 'SliderGradient',
              x1: '0%',
              y1: '0%',
              x2: '0%',
              y2: '100%'
            },
            '');
          defs.appendChild(linGrad);
          linGrad.appendChild(DomoticzDevicesService.makeSVGnode('stop',
            {
              offset: '0%',
              style: 'stop-color:#11AAFD;stop-opacity:1'
            },
            ''));
          linGrad.appendChild(DomoticzDevicesService.makeSVGnode('stop',
            {
              offset: '100%',
              style: 'stop-color:#0199E0;stop-opacity:1'
            },
            ''));
          let imgPattern = DomoticzDevicesService.makeSVGnode('pattern',
            {
              id: 'SliderImage',
              width: DomoticzDevicesService.Device.elementPadding * 2,
              height: DomoticzDevicesService.Device.elementPadding * 2,
              patternUnits: 'userSpaceOnUse'
            },
            '');
          defs.appendChild(imgPattern);
          imgPattern.appendChild(DomoticzDevicesService.makeSVGnode('image',
            {
              'xlink:href': 'css/images/bg-track.png',
              width: DomoticzDevicesService.Device.elementPadding * 2,
              height: DomoticzDevicesService.Device.elementPadding * 2
            },
            ''));
          imgPattern = DomoticzDevicesService.makeSVGnode('pattern',
            {
              id: 'ButtonImage1',
              width: DomoticzDevicesService.Device.elementPadding * 2,
              height: DomoticzDevicesService.Device.elementPadding * 2,
              patternUnits: 'userSpaceOnUse'
            },
            '');
          defs.appendChild(imgPattern);
          imgPattern.appendChild(DomoticzDevicesService.makeSVGnode('image',
            {
              'xlink:href': 'css/images/img03b.jpg',
              transform: 'rotate(180,8,17)',
              width: 16,
              height: 37
            },
            ''));
          imgPattern = DomoticzDevicesService.makeSVGnode('pattern',
            {
              id: 'ButtonImage2',
              width: DomoticzDevicesService.Device.elementPadding * 2,
              height: DomoticzDevicesService.Device.elementPadding * 2,
              patternUnits: 'userSpaceOnUse'
            },
            '');
          defs.appendChild(imgPattern);
          imgPattern.appendChild(DomoticzDevicesService.makeSVGnode('image',
            {
              'xlink:href': 'css/images/img03bs.jpg',
              width: 16,
              height: 37
            },
            ''));
        }
      }
    }
  }

  create(item: any) {
    let dev;
    let type = '';

    // if we got a string instead of an object then convert it
    if (typeof item === 'string') {
      item = JSON.parse(item);
    }

    // Anomalies in device pattern (Scenes & Dusk sensors say they are  lights(???)
    if (item.Type === 'Scene') {
      type = 'scene';
    } else if (item.Type === 'Group') {
      type = 'group';
    } else if ((item.Type === 'General') && (item.SubType === 'Barometer')) {
      type = 'baro';
    } else if ((item.Type === 'General') && (item.SubType === 'Custom Sensor')) {
      type = 'custom';
    } else if (
      (item.SwitchType === 'Dusk Sensor') ||
      (item.SwitchType === 'Selector')
    ) {
      type = item.SwitchType.toLowerCase();
    } else {
      type = item.TypeImg.toLowerCase();
      if ((item.CustomImage !== 0) && (typeof item.Image !== 'undefined')) {
        type = item.Image.toLowerCase();
      }
    }
    switch (type) {
      case 'alert':
        dev = new Alert(this.thisService, item);
        break;
      case 'baro':
        dev = new Baro(this.thisService, item);
        break;
      case 'blinds':
      case 'blinds inverted':
      case 'blinds percentage':
        dev = new Blinds(this.thisService, item);
        break;
      case 'contact':
        dev = new Contact(this.thisService, item);
        break;
      case 'counter':
        dev = new Counter(this.thisService, item);
        break;
      case 'current':
        dev = new Current(this.thisService, item);
        break;
      case 'custom':
        dev = new Custom(this.thisService, item);
        break;
      case 'dimmer':
        dev = new Dimmer(this.thisService, item);
        break;
      case 'door':
        dev = new Door(this.thisService, item);
        break;
      case 'door contact':
        dev = new DoorContact(this.thisService, item);
        break;
      case 'doorbell':
        dev = new Doorbell(this.thisService, item);
        break;
      case 'dusk sensor':
        dev = new DuskSensor(this.thisService, item);
        break;
      case 'fan':
        dev = new BinarySwitch(this.thisService, item);
        break;
      case 'humidity':
        dev = new Humidity(this.thisService, item);
        break;
      case 'lightbulb':
        dev = new Lightbulb(this.thisService, item);
        break;
      case 'lux':
        dev = new VariableSensor(this.thisService, item);
        break;
      case 'motion':
        dev = new Motion(this.thisService, item);
        break;
      case 'media':
        dev = new MediaPlayer(this.thisService, item);
        break;
      case 'group':
        dev = new Group(this.thisService, item);
        break;
      case 'hardware':
        dev = new Hardware(this.thisService, item);
        break;
      case 'push':
        dev = new Pushon(this.thisService, item);
        break;
      case 'pushoff':
        dev = new Pushoff(this.thisService, item);
        break;
      case 'override':
      case 'override_mini':
        dev = new SetPoint(this.thisService, item);
        break;
      case 'radiation':
        dev = new Radiation(this.thisService, item);
        break;
      case 'rain':
        dev = new Rain(this.thisService, item);
        break;
      case 'scene':
        dev = new Scene(this.thisService, item);
        break;
      case 'siren':
        dev = new Siren(this.thisService, item);
        break;
      case 'smoke':
        dev = new Smoke(this.thisService, item);
        break;
      case 'speaker':
        dev = new Dimmer(this.thisService, item);
        break;
      case 'temp':
      case 'temperature':
        dev = new Temperature(this.thisService, item);
        break;
      case 'text':
        dev = new Text(this.thisService, item);
        break;
      case 'visibility':
        dev = new Visibility(this.thisService, item);
        break;
      case 'venetian blinds':
        dev = new Blinds(this.thisService, item);
        break;
      case 'wind':
        dev = new Wind(this.thisService, item);
        break;
      case 'selector':
        dev = new Selector(this.thisService, item);
        break;
      default:
        // unknown image type (or user custom image)  so use switch type to determine what to show
        if (typeof item.SwitchType !== 'undefined') {
          switch (item.SwitchType.toLowerCase()) {
            case 'media player':
              dev = new MediaPlayer(this.thisService, item);
              break;
            default:
              dev = new Switch(this.thisService, item);
              break;
          }
        } else {
          dev = new VariableSensor(this.thisService, item);
        }
    }

    return dev;
  }

  htmlMinimum(parent): HTMLElement | SVGElement {
    let mainEl: HTMLElement | SVGElement = document.getElementById(this.uniquename);
    if (mainEl === undefined || mainEl === null) {
      if (this.moveable === true) {
        mainEl = DomoticzDevicesService.makeSVGnode('g', {
          id: this.uniquename,
          'class': 'Device_' + this.index,
          transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')',
          idx: this.index,
          xoffset: this.xoffset,
          yoffset: this.yoffset,
          devscenetype: this.devSceneType,
          onfloorplan: this.onFloorplan,
          opacity: (this.onFloorplan === true) ? '' : '0.5'
        }, '');
      } else {
        mainEl = DomoticzDevicesService.makeSVGnode('g', {
          id: this.uniquename,
          'class': 'Device_' + this.index,
          transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')'
        }, '');
      }
      this.drawIcon(mainEl);
      ($('#DeviceIcons')[0] === undefined || $('#DeviceIcons')[0] === null) ?
        parent.appendChild(mainEl) :
        $('#DeviceIcons')[0].appendChild(mainEl);

      if (this.moveable === true) {
        mainEl = DomoticzDevicesService.makeSVGnode('g', {
          id: this.uniquename,
          transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')',
          idx: this.index,
          xoffset: this.xoffset,
          yoffset: this.yoffset
        }, '');
      } else {
        mainEl = DomoticzDevicesService.makeSVGnode('g', {
          id: this.uniquename,
          transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')'
        }, '');
      }
      const oDetail = this.drawDetails(mainEl, false);
      ($('#DeviceDetails')[0] === undefined || $('#DeviceDetails')[0] === null) ?
        parent.appendChild(mainEl) :
        $('#DeviceDetails')[0].appendChild(mainEl);
      this.drawButtons(oDetail);
    } else {
      this.drawIcon(mainEl);
      const oDetail = this.drawDetails(mainEl, false);
      this.drawButtons(oDetail);
    }

    if (this.haveDimmer === true) {
      const oSlider = new Slider(this.thisService);
    }

    return mainEl;
  }

  drawIcon(parent: HTMLElement | SVGElement): SVGElement {
    let el: SVGElement;
    // Draw device values if option(s) is turned on
    if ((this.showStatus === true) && (this.smallStatus.length > 0)) {
      let nbackcolor = '#D4E1EE';
      if (this.protected === true) {
        nbackcolor = '#A4B1EE';
      }
      if (this.haveTimeout === true) {
        nbackcolor = '#DF2D3A';
      }
      if (DomoticzDevicesService.Device.useSVGtags === true) {
        let oText;
        if (this.hasNewLine) {
          oText = DomoticzDevicesService.makeSVGmultiline(
            {
              transform: 'translate(' + DomoticzDevicesService.Device.iconSize / 2 + ',' +
                (DomoticzDevicesService.Device.iconSize + (DomoticzDevicesService.Device.elementPadding * 1.5) + 1) + ')',
              'text-anchor': 'middle',
              'font-weight': 'bold',
              'font-size': '80%'
            },
            this.thisService.translationService.t(this.smallStatus.replace('Watt', 'W')),
            '',
            DomoticzDevicesService.Device.iconSize * 3,
            0,
            DomoticzDevicesService.Device.elementPadding * 2.5 + 1,
            '<br />'
          );
        } else {
          oText = DomoticzDevicesService.makeSVGmultiline(
            {
              transform: 'translate(' + DomoticzDevicesService.Device.iconSize / 2 + ',' +
                (DomoticzDevicesService.Device.iconSize + (DomoticzDevicesService.Device.elementPadding * 1.5) + 1) + ')',
              'text-anchor': 'middle',
              'font-weight': 'bold',
              'font-size': '80%'
            },
            this.thisService.translationService.t(this.smallStatus.replace('Watt', 'W')),
            '',
            DomoticzDevicesService.Device.iconSize * 3,
            0,
            DomoticzDevicesService.Device.elementPadding * 2.5 + 1
          );
        }

        const maxSpan = DomoticzDevicesService.getMaxSpanWidth(oText);
        el = DomoticzDevicesService.makeSVGnode('g', {
          id: this.uniquename + '_Tile',
          'class': 'DeviceTile',
          style: (maxSpan === 0) ? 'display:none' : 'display:inline'
        }, '');
        const offset = (DomoticzDevicesService.Device.iconSize / 2) - (maxSpan / 2) - DomoticzDevicesService.Device.elementPadding;
        el.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
          x: offset + 2,
          y: DomoticzDevicesService.Device.iconSize - (DomoticzDevicesService.Device.elementPadding * 0.5) + 2,
          rx: DomoticzDevicesService.Device.elementPadding,
          ry: DomoticzDevicesService.Device.elementPadding,
          width: maxSpan + (DomoticzDevicesService.Device.elementPadding * 2),
          height: oText.childNodes.length * DomoticzDevicesService.Device.elementPadding * 3,
          'stroke-width': '0',
          fill: 'black',
          opacity: '0.5'
        }, ''));
        el.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
          'class': 'header',
          x: offset,
          y: DomoticzDevicesService.Device.iconSize - (DomoticzDevicesService.Device.elementPadding * 0.5),
          rx: DomoticzDevicesService.Device.elementPadding,
          ry: DomoticzDevicesService.Device.elementPadding,
          width: maxSpan + (DomoticzDevicesService.Device.elementPadding * 2),
          height: oText.childNodes.length * DomoticzDevicesService.Device.elementPadding * 3,
          style: 'fill:' + nbackcolor
        }, ''));
        el.appendChild(oText);
      } else {
        // html
      }

      const existing = document.getElementById(this.uniquename + '_Tile');
      if (existing !== undefined && existing !== null) {
        existing.parentNode.replaceChild(el, existing);
      } else {
        parent.appendChild(el);
      }
    }

    if (DomoticzDevicesService.Device.useSVGtags === true) {
      el = DomoticzDevicesService.makeSVGnode('image', {
        id: this.uniquename + '_Icon',
        'class': 'DeviceIcon',
        'xlink:href': this.image,
        width: DomoticzDevicesService.Device.iconSize,
        height: DomoticzDevicesService.Device.iconSize,
        style: (this.moveable === true) ? 'cursor:move;' : 'cursor:hand; -webkit-user-select: none;'
      }, '');
      el.onmouseover = () => {
        if (this.moveable !== true) {
          DomoticzDevicesService.Device.popup(this.uniquename);
        }
      };
      el.onmouseout = () => {
        if (this.moveable !== true) {
          DomoticzDevicesService.Device.popupCancelDelay();
        }
      };
      el.onclick = () => {
        if ((this.moveable !== true) && (this.onClick !== null)) {
          DomoticzDevicesService.Device.popupCancelDelay();
          if (!this.controlable) {
            $('body').trigger('pageexit');
          }
          this.onClick();
        }
      };
      el.ontouchstart = () => {
        if (this.moveable !== true) {
          DomoticzDevicesService.Device.ignoreClick = true;
          DomoticzDevicesService.Device.popup(this.uniquename);
        }
      };
      el.ontouchend = () => {
        if (this.moveable !== true) {
          DomoticzDevicesService.Device.popupCancelDelay();
        }
      };
      el.appendChild(DomoticzDevicesService.makeSVGnode('title', null, this.name));
    } else {
      el = DomoticzDevicesService.makeSVGnode('img', {
        id: this.uniquename + '_Icon',
        'src': this.image,
        alt: this.name,
        width: DomoticzDevicesService.Device.iconSize,
        height: DomoticzDevicesService.Device.iconSize,
        style: (this.moveable === true) ? 'cursor:move;' : 'cursor:hand;'
      }, '');
      el.onmouseover = () => {
        if (this.moveable !== true) {
          DomoticzDevicesService.Device.popup(this.uniquename);
        }
      };
      el.onclick = () => {
        if (this.moveable !== true) {
          DomoticzDevicesService.Device.popup(this.uniquename);
        }
      };
    }

    const existing2 = document.getElementById(this.uniquename + '_Icon');
    if (existing2 !== undefined && existing2 !== null) {
      existing2.parentNode.replaceChild(el, existing2);
    } else {
      parent.appendChild(el);
    }

    return el;
  }

  drawDetails(parent?: SVGElement | HTMLElement, display?: boolean): HTMLElement | SVGElement {
    let nbackcolor = '#D4E1EE';
    const showme = (display === false) ? 'none' : 'inline';
    if (this.protected === true) {
      nbackcolor = '#A4B1EE';
    }
    if (this.haveTimeout === true) {
      nbackcolor = '#DF2D3A';
    }
    if (this.batteryLevel <= 10) {
      nbackcolor = '#DDDF2D';
    }
    const existing = document.getElementById(this.uniquename + '_Detail');
    let el: HTMLElement | SVGElement;
    let sDirection = 'right';
    if (DomoticzDevicesService.Device.useSVGtags === true) {
      if (existing !== undefined && existing !== null) {
        el = <HTMLElement>existing.cloneNode(false);  // shallow clone only
        sDirection = el.getAttribute('direction');
        const oTransform = new Transform(el);
        if (sDirection === 'right') {
          oTransform.xOffset = -DomoticzDevicesService.Device.elementPadding;
        } else {
          oTransform.xOffset = -(this.width - DomoticzDevicesService.Device.iconSize - DomoticzDevicesService.Device.elementPadding);
        }
        el.setAttribute('transform', oTransform.toString());
      } else {
        el = DomoticzDevicesService.makeSVGnode('g', {
          'class': 'DeviceDetails',
          id: this.uniquename + '_Detail',
          transform: 'translate(-' + DomoticzDevicesService.Device.elementPadding + ',-' +
            DomoticzDevicesService.Device.elementPadding * 6 + ')',
          width: this.width,
          height: this.height,
          direction: 'right',
          style: 'display:' + showme + '; -webkit-user-select: none;',
          'pointer-events': 'none'
        }, '');
        el.onmouseleave = () => {
          $('.DeviceDetails').css('display', 'none');
        };
      }
      el.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
        id: 'shadow',
        transform: 'translate(2,2)',
        rx: DomoticzDevicesService.Device.elementPadding,
        ry: DomoticzDevicesService.Device.elementPadding,
        width: this.width,
        height: (el.getAttribute('expanded') !== 'true') ? this.height : this.height + (DomoticzDevicesService.Device.elementPadding * 6),
        'stroke-width': '0',
        fill: 'black',
        opacity: '0.3'
      }, ''));
      el.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
        'class': 'popup',
        rx: DomoticzDevicesService.Device.elementPadding,
        ry: DomoticzDevicesService.Device.elementPadding,
        width: this.width,
        height: (el.getAttribute('expanded') !== 'true') ? this.height : this.height + (DomoticzDevicesService.Device.elementPadding * 6),
        stroke: 'gray',
        'stroke-width': '0.25',
        fill: 'url(#PopupGradient)',
        'pointer-events': 'all'
      }, ''));
      el.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
        'class': 'header',
        x: DomoticzDevicesService.Device.elementPadding,
        y: DomoticzDevicesService.Device.elementPadding,
        rx: DomoticzDevicesService.Device.elementPadding,
        ry: DomoticzDevicesService.Device.elementPadding,
        width: (this.width - (DomoticzDevicesService.Device.elementPadding * 2)),
        height: DomoticzDevicesService.Device.elementPadding * 4,
        style: 'fill:' + nbackcolor
      }, ''));
      if (this.haveCamera === true) {
        el.appendChild(DomoticzDevicesService.makeSVGnode('image', {
          id: 'webcam',
          'xlink:href': 'images/webcam.png',
          width: 16,
          height: 16,
          x: (this.width - (DomoticzDevicesService.Device.elementPadding * 2) - 16),
          y: (DomoticzDevicesService.Device.elementPadding * 3) - 8,
          'pointer-events': 'all'
        }, '', this.thisService.translationService.t('Stream Video')));
        el.onclick = () => {
          if (this.WebcamClick !== null) {
            this.WebcamClick();
          }
        };
        el.onmouseover = () => {
          Utils.cursorhand();
        };
        el.onmouseout = () => {
          Utils.cursordefault();
        };
      }
      el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
        id: 'name',
        x: DomoticzDevicesService.Device.elementPadding * 2,
        y: DomoticzDevicesService.Device.elementPadding * 4,
        'text-anchor': 'start'
      }, this.name));
      el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
        id: 'bigtext',
        x: (this.width - (DomoticzDevicesService.Device.elementPadding * 2)),
        y: DomoticzDevicesService.Device.elementPadding * 4,
        'text-anchor': 'end',
        'font-weight': 'bold'
      }, this.thisService.translationService.t(this.data)));

      let iOffset = ((sDirection === 'right') ? DomoticzDevicesService.Device.elementPadding :
        ((this.image2 === '') ? this.width - DomoticzDevicesService.Device.elementPadding -
          DomoticzDevicesService.Device.iconSize : this.width -
          (DomoticzDevicesService.Device.elementPadding * 2) - (DomoticzDevicesService.Device.iconSize * 2)));
      const gImageGroup = DomoticzDevicesService.makeSVGnode('g', {
        id: 'imagegroup',
        transform: 'translate(' + iOffset + ',' + DomoticzDevicesService.Device.elementPadding * 6 + ')'
      }, '');
      el.appendChild(gImageGroup);
      const svgElt = DomoticzDevicesService.makeSVGnode('image', {
        id: 'image',
        'xlink:href': this.image,
        width: DomoticzDevicesService.Device.iconSize,
        height: DomoticzDevicesService.Device.iconSize,
        opacity: this.image_opacity,
        'pointer-events': 'all'
      }, '', this.thisService.translationService.t(this.imagetext));
      svgElt.onclick = () => {
        if (this.onClick !== null) {
          if (DomoticzDevicesService.Device.ignoreClick !== true) {
            if (!this.controlable) {
              $('body').trigger('pageexit');
            }
            this.onClick();
          }
        }
      };
      svgElt.onmouseover = () => {
        if (this.onClick !== null) {
          Utils.cursorhand();
        }
      };
      svgElt.onmouseout = () => {
        if (this.onClick !== null) {
          Utils.cursordefault();
        }
      };
      gImageGroup.appendChild(svgElt);
      if (this.image2 !== '') {
        const svgElt2 = DomoticzDevicesService.makeSVGnode('image', {
          id: 'image2',
          x: DomoticzDevicesService.Device.iconSize + DomoticzDevicesService.Device.elementPadding,
          'xlink:href': this.image2,
          width: DomoticzDevicesService.Device.iconSize,
          height: DomoticzDevicesService.Device.iconSize,
          opacity: this.image2_opacity,
          'pointer-events': 'all'
        }, '', this.thisService.translationService.t(this.imagetext));
        svgElt2.onclick = () => {
          if (this.onClick2 !== null) {
            this.onClick2();
          }
        };
        svgElt2.onmouseover = () => {
          if (this.onClick !== null) {
            Utils.cursorhand();
          }
        };
        svgElt2.onmouseout = () => {
          if (this.onClick !== null) {
            Utils.cursordefault();
          }
        };
        gImageGroup.appendChild(svgElt2);
      }
      iOffset = ((sDirection === 'right') ?
        ((this.image2 === '') ?
          DomoticzDevicesService.Device.iconSize + (DomoticzDevicesService.Device.elementPadding * 2) :
          (DomoticzDevicesService.Device.iconSize * 2 + DomoticzDevicesService.Device.elementPadding * 3)) :
        DomoticzDevicesService.Device.elementPadding * 2);
      const gStatusGroup = DomoticzDevicesService.makeSVGnode('g', {
        id: 'statusgroup',
        transform: 'translate(' + iOffset + ',' + DomoticzDevicesService.Device.elementPadding * 6 + ')'
      }, '');
      el.appendChild(gStatusGroup);
      if (this instanceof Selector) {
        this.drawCustomStatus(gStatusGroup);
      } else if (this.haveDimmer === true) {
        gStatusGroup.appendChild(DomoticzDevicesService.makeSVGnode('text', {
          id: 'status',
          x: 0,
          y: DomoticzDevicesService.Device.elementPadding * 2,
          'font-weight': 'bold',
          'font-size': '90%'
        }, this.thisService.deviceService.TranslateStatus(this.status)));
        gStatusGroup.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
          id: 'sliderback',
          'class': 'SliderBack',
          x: 0,
          y: DomoticzDevicesService.Device.elementPadding * 3,
          width: DomoticzDevicesService.Device.elementPadding * 35,
          height: DomoticzDevicesService.Device.elementPadding * 2,
          rx: DomoticzDevicesService.Device.elementPadding,
          ry: DomoticzDevicesService.Device.elementPadding,
          fill: 'url(#SliderImage)',
          stroke: 'black',
          'stroke-width': '0.5',
          'pointer-events': 'all'
        }, '', this.thisService.translationService.t('Adjust level')));
        gStatusGroup.appendChild(DomoticzDevicesService.makeSVGnode('rect', {
          id: 'slider',
          'class': 'Slider',
          x: 0,
          y: DomoticzDevicesService.Device.elementPadding * 3,
          width: (DomoticzDevicesService.Device.elementPadding * 35) * (this.level / this.levelMax),
          height: DomoticzDevicesService.Device.elementPadding * 2,
          rx: DomoticzDevicesService.Device.elementPadding,
          ry: DomoticzDevicesService.Device.elementPadding,
          fill: 'url(#SliderGradient)',
          stroke: 'black',
          'stroke-width': '0.5',
          'pointer-events': 'all'
        }, '', this.thisService.translationService.t('Adjust level')));
        const svgElt3 = DomoticzDevicesService.makeSVGnode('image', {
          id: 'sliderhandle',
          'class': 'SliderHandle',
          x: ((DomoticzDevicesService.Device.elementPadding * 35) * (this.level / this.levelMax)) -
            (DomoticzDevicesService.Device.elementPadding * 2),
          y: DomoticzDevicesService.Device.elementPadding * 2,
          level: this.level,
          maxlevel: this.levelMax,
          devindex: this.index,
          'xlink:href': 'images/handle.png',
          width: DomoticzDevicesService.Device.elementPadding * 4,
          height: DomoticzDevicesService.Device.elementPadding * 4,
          'pointer-events': 'all',
        }, '', this.thisService.translationService.t('Slide to adjust level'));
        svgElt3.onmouseover = () => {
          Utils.cursorhand();
        };
        svgElt3.onmouseout = () => {
          Utils.cursordefault();
        };
        gStatusGroup.appendChild(svgElt3);
      } else {
        let oStatus: SVGElement;
        if (this.hasNewLine) {
          oStatus = DomoticzDevicesService.makeSVGmultiline({
            id: 'status',
            transform: 'translate(0,' + DomoticzDevicesService.Device.elementPadding * 3 + ')',
            'font-weight': 'bold',
            'font-size': '90%'
          },
            this.thisService.deviceService.TranslateStatus(this.status),
            '',
            ((this.image2 === '') ? DomoticzDevicesService.Device.elementPadding * 37 : DomoticzDevicesService.Device.elementPadding * 32),
            DomoticzDevicesService.Device.elementPadding * -1.5,
            DomoticzDevicesService.Device.elementPadding * 3,
            '<br />');
        } else {
          oStatus = DomoticzDevicesService.makeSVGmultiline({
            id: 'status',
            transform: 'translate(0,' + DomoticzDevicesService.Device.elementPadding * 3 + ')',
            'font-weight': 'bold',
            'font-size': '90%'
          }, this.thisService.deviceService.TranslateStatus(this.status), '',
            ((this.image2 === '') ? DomoticzDevicesService.Device.elementPadding * 37 : DomoticzDevicesService.Device.elementPadding * 32),
            DomoticzDevicesService.Device.elementPadding * -1.5, DomoticzDevicesService.Device.elementPadding * 3);
          if (oStatus.childNodes.length > 2) { // if text is too long try and squeeze it. if that doesn't work chop it at 2 lines
            oStatus = DomoticzDevicesService.makeSVGmultiline({
              id: 'status',
              transform: 'translate(0,' + DomoticzDevicesService.Device.elementPadding * 3 + ')',
              'font-weight': 'bold',
              'font-size': '90%'
            }, this.thisService.deviceService.TranslateStatus(this.status), '',
              ((this.image2 === '') ? DomoticzDevicesService.Device.elementPadding * 37 :
                DomoticzDevicesService.Device.elementPadding * 32),
              DomoticzDevicesService.Device.elementPadding * -1.5, DomoticzDevicesService.Device.elementPadding * 3, ' ');
            while (oStatus.childNodes.length > 2) {
              oStatus.removeChild(oStatus.childNodes[oStatus.childNodes.length - 1]);
            }
          }
        }

        gStatusGroup.appendChild(oStatus);
      }
      const gText = DomoticzDevicesService.makeSVGnode('text', {
        id: 'lastseen',
        x: 0,
        y: DomoticzDevicesService.Device.elementPadding * 7.5,
        'font-size': '80%'
      }, '');
      gStatusGroup.appendChild(gText);
      gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
        id: 'lastlabel', 'font-style': 'italic', 'font-size': '80%'
      }, this.thisService.translationService.t('Last Seen')));
      gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
        id: 'lastlabel', 'font-style': 'italic', 'font-size': '80%'
      }, ':'));
      gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
        id: 'lastupdate', 'font-size': '80%'
      }, ' ' + this.lastupdate));
    } else {  // this is not used but is included to show that the code could draw other than SVG (such as HTML5 canvas)
      el = DomoticzDevicesService.makeSVGnode('div', {
        'class': 'span4 DeviceDetails', id: this.uniquename + '_Detail',
        style: 'display:' + showme + '; position:relative; '
      }, '');
      const table = DomoticzDevicesService.makeSVGnode('table', {
        'id': 'itemtablesmall', border: '0', cellspacing: '0', cellpadding: '0'
      }, '');
      el.appendChild(table);
      const tbody = DomoticzDevicesService.makeSVGnode('tbody', {}, '');
      table.appendChild(tbody);
      const tr = DomoticzDevicesService.makeSVGnode('tr', {}, '');
      tbody.appendChild(tr);
      tr.appendChild(DomoticzDevicesService.makeSVGnode('td', {
        id: 'name', style: 'background-color: rgb(164,177,238);'
      }, this.name));
      tr.appendChild(DomoticzDevicesService.makeSVGnode('td', { id: 'bigtext' }, this.data));
      tr.appendChild(DomoticzDevicesService.makeSVGnode('img', {
        id: 'image', src: this.image, width: DomoticzDevicesService.Device.iconSize, height: DomoticzDevicesService.Device.iconSize
      }, ''));
      tr.appendChild(DomoticzDevicesService.makeSVGnode('td', { id: 'status' }, this.status));
      tr.appendChild(DomoticzDevicesService.makeSVGnode('span', {
        id: 'lastseen', 'font-size': '80%', 'font-style': 'italic'
      }, 'Last Seen: '));
      tr.appendChild(DomoticzDevicesService.makeSVGnode('td', { id: 'lastupdate' }, this.lastupdate));
    }

    if (existing !== undefined && existing !== null) {
      existing.parentNode.replaceChild(el, existing);
    } else {
      parent.appendChild(el);
    }

    return el;
  }

  drawButtons(parent?: HTMLElement | SVGElement): SVGElement {
    let bVisible = false;
    let sDirection = 'right';
    if (typeof parent !== 'undefined') {
      bVisible = (parent.getAttribute('expanded') === 'true') ? true : false;
      sDirection = (parent.getAttribute('direction') === 'left') ? 'left' : 'right';
    }
    let el: SVGElement;
    let iOffset;
    if ((this.LogLink.length > 0) || (this.TimerLink.length > 0) || (this.NotifyLink.length > 0) || (this.forecastURL.length !== 0)) {
      if (DomoticzDevicesService.Device.useSVGtags === true) {
        iOffset = this.width - DomoticzDevicesService.Device.elementPadding - 16;
        if (sDirection === 'left') {
          iOffset = (this.image2 === '') ?
            this.width - DomoticzDevicesService.Device.elementPadding - DomoticzDevicesService.Device.iconSize - 16 :
            this.width - (DomoticzDevicesService.Device.elementPadding * 2) - (DomoticzDevicesService.Device.iconSize * 2) - 16;
        }
        let twistyRotate = '';
        if (bVisible === true) {
          twistyRotate = ' rotate(180,8,8)';
        }
        const svgNode1 = DomoticzDevicesService.makeSVGnode('image', {
          id: 'twisty',
          'xlink:href': 'images/expand16.png',
          width: 16,
          height: 16,
          transform: 'translate(' + iOffset + ',' + (this.height - DomoticzDevicesService.Device.elementPadding - 16) + ')' + twistyRotate,
          onmouseover: 'this.style.cursor = \'n-resize\';',
          'pointer-events': 'all'
        }, '', this.thisService.translationService.t('Details display'));
        svgNode1.onclick = () => {
          DomoticzDevicesService.Device.popupExpand(this.uniquename);
        };
        svgNode1.onmouseout = () => {
          Utils.cursordefault();
        };
        parent.appendChild(svgNode1);
        el = DomoticzDevicesService.makeSVGnode('g', {
          id: 'detailsgroup',
          transform: 'translate(0,' + DomoticzDevicesService.Device.elementPadding * 15 + ')',
          style: bVisible ? 'display:inline' : 'display:none'
        }, '');
        iOffset = ((sDirection === 'right') ?
          ((this.image2 === '') ? DomoticzDevicesService.Device.iconSize + (DomoticzDevicesService.Device.elementPadding * 2) :
            (DomoticzDevicesService.Device.iconSize * 2 + DomoticzDevicesService.Device.elementPadding * 3)) :
          DomoticzDevicesService.Device.elementPadding * 2);
        const gText = DomoticzDevicesService.makeSVGnode('text', {
          id: 'type', x: iOffset, y: DomoticzDevicesService.Device.elementPadding, 'font-size': '80%', 'font-style': 'italic'
        }, '');
        el.appendChild(gText);
        gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
          id: 'typelabel', 'font-size': '80%'
        }, this.thisService.translationService.t('Type')));
        gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
          id: 'typelabel', 'font-size': '80%'
        }, ':'));
        gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
          id: 'typedetail1', 'font-size': '80%'
        }, ' ' + this.type));
        if (typeof this.subtype !== 'undefined') {
          gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
            id: 'typedetail2', 'font-size': '80%'
          }, ', ' + this.subtype));
        }
        if (typeof this.switchType !== 'undefined') {
          gText.appendChild(DomoticzDevicesService.makeSVGnode('tspan', {
            id: 'typedetail3', 'font-size': '80%'
          }, ', ' + this.switchType));
        }
        if (this.thisService.permissionService.getRights() === 2) {
          const svgNode2 = DomoticzDevicesService.makeSVGnode('image', {
            id: 'favorite',
            x: DomoticzDevicesService.Device.elementPadding,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            'xlink:href': (this.favorite === 1) ? 'images/favorite.png' : 'images/nofavorite.png',
            width: '16',
            height: '16',
            'pointer-events': 'all'
          }, '', this.thisService.translationService.t('Toggle dashboard display'));
          svgNode2.onclick = () => {
            if (this.favorite === 1) {
              DomoticzDevicesService.Device.MakeFavorite(this.index, 0);
            } else {
              DomoticzDevicesService.Device.MakeFavorite(this.index, 1);
            }
          };
          svgNode2.onmouseover = () => {
            Utils.cursorhand();
          };
          svgNode2.onmouseout = () => {
            Utils.cursordefault();
          };
          el.appendChild(svgNode2);
        } else {
          el.appendChild(DomoticzDevicesService.makeSVGnode('image', {
            id: 'favorite',
            x: DomoticzDevicesService.Device.elementPadding,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            'xlink:href': (this.favorite === 1) ? 'images/favorite.png' : 'images/nofavorite.png',
            width: '16', height: '16'
          }, '', this.thisService.translationService.t('Favorite')));
        }
        let iLength = 0;
        iOffset = DomoticzDevicesService.Device.elementPadding * 5;
        if (this.LogLink.length > 0) {
          iLength = Math.floor(DomoticzDevicesService.SVGTextLength({
            'text-anchor': 'middle', 'font-size': '75%', fill: 'white'
          }, this.thisService.translationService.t('Log'))) + (DomoticzDevicesService.Device.elementPadding * 2);
          const svgNode4 = DomoticzDevicesService.makeSVGnode('rect', {
            id: 'Log',
            x: iOffset,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            width: iLength,
            height: DomoticzDevicesService.Device.elementPadding * 3,
            rx: DomoticzDevicesService.Device.elementPadding,
            ry: DomoticzDevicesService.Device.elementPadding,
            fill: 'url(#ButtonImage1)',
            'pointer-events': 'all'
          }, this.thisService.translationService.t('Log'), '');
          svgNode4.onclick = () => {
            if (this.LogLink.length !== 0) {
              $('body').trigger('pageexit');
              this.thisService.router.navigate(this.LogLink);
            }
          };
          svgNode4.onmouseover = () => {
            if (this.LogLink.length !== 0) {
              Utils.cursorhand();
            }
          };
          svgNode4.onmouseout = () => {
            if (this.LogLink.length !== 0) {
              Utils.cursordefault();
            }
          };
          el.appendChild(svgNode4);
          el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
            x: iOffset + (iLength / 2),
            y: DomoticzDevicesService.Device.elementPadding * 4,
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Log'), ''));
          iOffset += iLength + DomoticzDevicesService.Device.elementPadding;
        }
        if ((this.thisService.permissionService.getRights() === 2) && (this.EditLink.length > 0)) {
          iLength = Math.floor(DomoticzDevicesService.SVGTextLength({
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Edit'))) + (DomoticzDevicesService.Device.elementPadding * 2);
          const svgNode5 = DomoticzDevicesService.makeSVGnode('rect', {
            id: 'Edit',
            x: iOffset,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            width: iLength,
            height: DomoticzDevicesService.Device.elementPadding * 3,
            rx: DomoticzDevicesService.Device.elementPadding,
            ry: DomoticzDevicesService.Device.elementPadding,
            fill: 'url(#ButtonImage1)',
            'pointer-events': 'all'
          }, this.thisService.translationService.t('Edit'), '');
          svgNode5.onclick = () => {
            if (this.EditLink.length !== 0) {
              $('body').trigger('pageexit');
              this.thisService.router.navigate(this.EditLink);
            }
          };
          svgNode5.onmouseover = () => {
            if (this.EditLink.length !== 0) {
              Utils.cursorhand();
            }
          };
          svgNode5.onmouseout = () => {
            if (this.EditLink.length !== 0) {
              Utils.cursordefault();
            }
          };
          el.appendChild(svgNode5);
          el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
            x: iOffset + (iLength / 2),
            y: DomoticzDevicesService.Device.elementPadding * 4,
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Edit'), ''));
          iOffset += iLength + DomoticzDevicesService.Device.elementPadding;
        }
        if (this.TimerLink.length > 0) {
          iLength = Math.floor(DomoticzDevicesService.SVGTextLength({
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Timers'))) + (DomoticzDevicesService.Device.elementPadding * 2);
          const svgNode6 = DomoticzDevicesService.makeSVGnode('rect', {
            id: 'Timers',
            x: iOffset,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            width: iLength,
            height: DomoticzDevicesService.Device.elementPadding * 3,
            rx: DomoticzDevicesService.Device.elementPadding,
            ry: DomoticzDevicesService.Device.elementPadding,
            fill: 'url(#ButtonImage1)',
            'ng-controller': 'ScenesController',
            'pointer-events': 'all'
          }, this.thisService.translationService.t('Timers'), '');
          svgNode6.onclick = () => {
            if (this.TimerLink.length !== 0) {
              $('body').trigger('pageexit');
              this.thisService.router.navigate(this.TimerLink);
            }
          };
          svgNode6.onmouseover = () => {
            if (this.TimerLink.length !== 0) {
              Utils.cursorhand();
            }
          };
          svgNode6.onmouseout = () => {
            if (this.TimerLink.length !== 0) {
              Utils.cursordefault();
            }
          };
          el.appendChild(svgNode6);
          el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
            x: iOffset + (iLength / 2),
            y: DomoticzDevicesService.Device.elementPadding * 4,
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Timers'), ''));
          iOffset += iLength + DomoticzDevicesService.Device.elementPadding;
        }
        if ((this.thisService.permissionService.getRights() === 2) && (this.NotifyLink.length > 0)) {
          iLength = Math.floor(DomoticzDevicesService.SVGTextLength({
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Notifications'))) + (DomoticzDevicesService.Device.elementPadding * 2);
          const svgNode7 = DomoticzDevicesService.makeSVGnode('rect', {
            id: 'Notifications',
            x: iOffset,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            width: iLength,
            height: DomoticzDevicesService.Device.elementPadding * 3,
            rx: DomoticzDevicesService.Device.elementPadding,
            ry: DomoticzDevicesService.Device.elementPadding,
            fill: (this.hasNotifications === true) ? 'url(#ButtonImage2)' : 'url(#ButtonImage1)',
            'pointer-events': 'all'
          }, this.thisService.translationService.t('Notifications'), '');
          svgNode7.onclick = () => {
            if (this.NotifyLink.length !== 0) {
              $('body').trigger('pageexit');
              this.thisService.router.navigate(this.NotifyLink);
            }
          };
          svgNode7.onmouseover = () => {
            if (this.NotifyLink.length !== 0) {
              Utils.cursorhand();
            }
          };
          svgNode7.onmouseout = () => {
            if (this.NotifyLink.length !== 0) {
              Utils.cursordefault();
            }
          };
          el.appendChild(svgNode7);
          el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
            x: iOffset + (iLength / 2),
            y: DomoticzDevicesService.Device.elementPadding * 4,
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Notifications'), ''));
          iOffset += iLength + DomoticzDevicesService.Device.elementPadding;
        }
        if (this.forecastURL.length !== 0) {
          iLength = Math.floor(DomoticzDevicesService.SVGTextLength({
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Forecast'))) + (DomoticzDevicesService.Device.elementPadding * 2);
          const svgNode8 = DomoticzDevicesService.makeSVGnode('rect', {
            id: 'Forecast',
            x: iOffset,
            y: DomoticzDevicesService.Device.elementPadding * 2,
            width: iLength,
            height: DomoticzDevicesService.Device.elementPadding * 3,
            rx: DomoticzDevicesService.Device.elementPadding,
            ry: DomoticzDevicesService.Device.elementPadding,
            fill: 'url(#ButtonImage1)',
            'pointer-events': 'all'
          }, this.thisService.translationService.t('Forecast'), '');
          svgNode8.onclick = () => {
            window.open(Utils.decodeBase64(this.forecastURL));
          };
          svgNode8.onmouseover = () => {
            Utils.cursorhand();
          };
          svgNode8.onmouseout = () => {
            Utils.cursordefault();
          };
          el.appendChild(svgNode8);
          el.appendChild(DomoticzDevicesService.makeSVGnode('text', {
            x: iOffset + (iLength / 2),
            y: DomoticzDevicesService.Device.elementPadding * 4,
            'text-anchor': 'middle',
            'font-size': '75%',
            fill: 'white'
          }, this.thisService.translationService.t('Forecast'), ''));
          iOffset += iLength + DomoticzDevicesService.Device.elementPadding;
        }
      } else {
        // insert HTML here
      }

      if (typeof parent !== 'undefined') {
        parent.appendChild(el);
      }
    }

    return el;
  }

  popup(target) {
    const ignorePopupDelay = (DomoticzDevicesService.Device.popupDelayDevice !== '');
    DomoticzDevicesService.Device.popupCancelDelay();
    $('.DeviceDetails').css('display', 'none');   // hide all popups
    if (DomoticzDevicesService.Device.expandVar != null) {
      DomoticzDevicesService.Device.expandVar.unsubscribe();
      DomoticzDevicesService.Device.expandVar = null;
    }
    DomoticzDevicesService.Device.expandRect = null;
    DomoticzDevicesService.Device.expandShadow = null;

    // Defer showing popup if a delay has been specified
    if ((ignorePopupDelay === false) && (DomoticzDevicesService.Device.popupDelay >= 0)) {
      DomoticzDevicesService.Device.popupDelayDevice = target;
      DomoticzDevicesService.Device.popupDelayTimer = timer(DomoticzDevicesService.Device.popupDelay).subscribe(() => {
        DomoticzDevicesService.Device.popup(DomoticzDevicesService.Device.popupDelayDevice);
      });
      return;
    }

    // try to check if the popup needs to be flipped, if this can't be done just display it
    const oTarget = document.getElementById(target + '_Detail');
    let oTemp = oTarget.parentNode;
    const oTransform = new Transform();
    while (oTemp.nodeName.toLowerCase() !== 'svg') {
      const oNew = new Transform(oTemp);
      oTransform.Add(oNew);
      oTemp = oTemp.parentNode;
    }
    oTarget.setAttribute('style', 'display:inline; -webkit-user-select: none;');

    const imageWidth = (oTransform.scale === 0) ?
      DomoticzDevicesService.Device.xImageSize :
      DomoticzDevicesService.Device.xImageSize / oTransform.scale;
    const requiredWidth = oTransform.xOffset + Number(oTarget.getAttribute('width'));
    const sDir = (imageWidth < requiredWidth) ? 'left' : 'right';
    if (sDir !== oTarget.getAttribute('direction')) {
      oTarget.setAttribute('direction', sDir);
    }
    oTarget.setAttribute('expanded', 'false');
    DomoticzDevicesService.Device.popupRedraw(target);
  }

  popupCancelDelay() {
    if (DomoticzDevicesService.Device.popupDelayTimer !== 0) {
      (<Subscription>DomoticzDevicesService.Device.popupDelayTimer).unsubscribe();
    }
    DomoticzDevicesService.Device.popupDelayDevice = '';
    DomoticzDevicesService.Device.popupDelayTimer = 0;
  }

  popupRedraw(target) {
    const oData = document.getElementById(target + '_Data');
    if ((typeof oData !== 'undefined') && (oData != null)) {
      const sData = oData.getAttribute('item');
      const oDevice = DomoticzDevicesService.Device.create(sData);
      const oTarget = document.getElementById(target + '_Detail');
      if (typeof oTarget !== 'undefined') {
        DomoticzDevicesService.Device.checkDefs();
        oDevice.htmlMinimum(oTarget.parentNode);
      }
    }
    return;
  }

  popupExpand(target) {
    const oTarget: any = document.getElementById(target + '_Detail');
    if ((typeof oTarget !== 'undefined') && (oTarget != null)) {
      DomoticzDevicesService.Device.expandTarget = target;
      let oDetails;
      for (let i = 0; i < oTarget.childNodes.length; i++) {
        if (oTarget.childNodes[i].className.baseVal === 'popup') {
          DomoticzDevicesService.Device.expandRect = oTarget.childNodes[i];
        }
        if (oTarget.childNodes[i].id === 'shadow') {
          DomoticzDevicesService.Device.expandShadow = oTarget.childNodes[i];
        }
        if (oTarget.childNodes[i].id === 'twisty') {
          DomoticzDevicesService.Device.expandRot = oTarget.childNodes[i];
          const oTransform = new Transform(DomoticzDevicesService.Device.expandRot);
          if (oTransform.rotation === 0) {
            oTransform.rotation = 1;
          }
          oTransform.xRotation = 8;
          oTransform.yRotation = 8;
          DomoticzDevicesService.Device.expandRot.setAttribute('transform', oTransform.toString());
        }
        if (oTarget.childNodes[i].id === 'detailsgroup') {
          oDetails = oTarget.childNodes[i];
        }
      }
      if (oTarget.getAttribute('expanded') === 'true') {
        DomoticzDevicesService.Device.expandInc = -1;
        DomoticzDevicesService.Device.expandEnd = 75;
        if (typeof oDetails !== 'undefined') {
          oDetails.setAttribute('style', 'display:none');
        }
        oTarget.setAttribute('expanded', 'false');
      } else {
        DomoticzDevicesService.Device.expandInc = 1;
        DomoticzDevicesService.Device.expandEnd = 105;
        oTarget.setAttribute('expanded', 'true');
      }
      DomoticzDevicesService.Device.expandInt = Math.round(400 / (105 - 75));
      DomoticzDevicesService.Device.expandVar = interval(DomoticzDevicesService.Device.expandInt).subscribe(() => {
        DomoticzDevicesService.Device.popupExpansion();
      });
    }
  }

  popupExpansion() {
    if ((DomoticzDevicesService.Device.expandRect == null) || (DomoticzDevicesService.Device.expandShadow == null)) {
      // this should never happen so just stop everything if it does !
      if (DomoticzDevicesService.Device.expandVar != null) {
        DomoticzDevicesService.Device.expandVar.unsubscribe();
        DomoticzDevicesService.Device.expandVar = null;
      }
      return;
    }
    const newHeight = Number(DomoticzDevicesService.Device.expandRect.getAttribute('height')) + DomoticzDevicesService.Device.expandInc;
    DomoticzDevicesService.Device.expandRect.setAttribute('height', newHeight);
    DomoticzDevicesService.Device.expandShadow.setAttribute('height', newHeight + 2);
    const oTransform = new Transform(DomoticzDevicesService.Device.expandRot);
    oTransform.rotation += Math.round(180 / (105 - 75)) * DomoticzDevicesService.Device.expandInc;
    DomoticzDevicesService.Device.expandRot.setAttribute('transform', oTransform.toString());
    if (((DomoticzDevicesService.Device.expandInc < 0) && (newHeight <= DomoticzDevicesService.Device.expandEnd)) ||
      ((DomoticzDevicesService.Device.expandInc > 0) && (newHeight >= DomoticzDevicesService.Device.expandEnd))) {
      DomoticzDevicesService.Device.expandVar.unsubscribe();
      DomoticzDevicesService.Device.expandRect.setAttribute('height', DomoticzDevicesService.Device.expandEnd);
      DomoticzDevicesService.Device.expandShadow.setAttribute('height', DomoticzDevicesService.Device.expandEnd + 2);
      DomoticzDevicesService.Device.expandRect = null;
      DomoticzDevicesService.Device.expandShadow = null;
      DomoticzDevicesService.Device.expandVar = null;
      DomoticzDevicesService.Device.popupRedraw(DomoticzDevicesService.Device.expandTarget);
    }
  }

  setDraggable(draggable) {
    this.moveable = draggable;
  }

  htmlMedium() {
    return this.drawDetails();
  }

  htmlMaximum() {
    // FIXME what are we adding??
    return <any>this.drawDetails() + <any>this.drawButtons();
  }

  htmlMobile() {
    return this.drawDetails();
  }

  MakeFavorite(id: number, isfavorite: number) {
    if (this.thisService.permissionService.getRights() !== 2) {
      this.thisService.notificationService.HideNotify();
      this.thisService.notificationService.ShowNotify(
        this.thisService.translationService.t('You do not have permission to do that!'), 2500, true);
      return;
    }
    // clearInterval($.myglobals.refreshTimer);
    this.thisService.deviceService.makeFavorite(id, isfavorite).subscribe(() => {
      this.thisService.configService.LastUpdateTime = 0;
      DomoticzDevicesService.Device.switchFunction();
    });
  }

  // scale(attr) {
  //   if (typeof $('#DeviceContainer')[0] !== 'undefined') {
  //     $('#DeviceContainer')[0].setAttribute('transform', attr);
  //   }
  //   return;
  // }

  initialise() {
    DomoticzDevicesService.Device.count = 0;
    DomoticzDevicesService.Device.notPositioned = 0;
    if (DomoticzDevicesService.Device.useSVGtags === true) {
      // clean up existing data if it exists
      $('#DeviceData').empty();
      $('#DeviceDetails').empty();
      $('#DeviceIcons').empty();

      const cont = DomoticzDevicesService.getSVGnode();
      if (cont !== undefined) {
        if ($('#DeviceContainer')[0] === undefined) {
          const devCont = DomoticzDevicesService.makeSVGnode('g', {id: 'DeviceContainer'}, '');
          cont.appendChild(devCont);
          DomoticzDevicesService.Device.checkDefs();
          if ($('#DeviceIcons')[0] === undefined) {
            devCont.appendChild(DomoticzDevicesService.makeSVGnode('g', {id: 'DeviceIcons'}, ''));
          }
          if ($('#DeviceDetails')[0] === undefined) {
            devCont.appendChild(DomoticzDevicesService.makeSVGnode('g', {id: 'DeviceDetails'}, ''));
          }
          if ($('#DeviceData')[0] === undefined) {
            devCont.appendChild(DomoticzDevicesService.makeSVGnode('g', {id: 'DeviceData'}, ''));
          }
        }
      }
    }
  }

}



export class Sensor extends DomoticzDevice {

  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/' + item.TypeImg + '48.png';

      const sensorType = this.type.replace(/\s/g, '');

      if (sensorType === 'General') {
        this.LogLink = ['/Devices', this.index, 'Log'];
      } else {
        this.LogLink = ['/' + sensorType + 'Log', this.index];
        this.onClick = () => {
          thisService.router.navigate(this.LogLink);
        };
      }

      this.imagetext = 'Show graph';
      this.NotifyLink = ['/Devices', this.index, 'Notifications'];

      if (this.haveCamera === true) {
        this.WebcamClick = () => {
          thisService.ShowCameraLiveStream(this.name, this.cameraIdx);
        };
      }
      this.showStatus = (DomoticzDevicesService.Device.showSensorValues === true);
    }
  }

}

export class VariableSensor extends Sensor {

  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
    }
  }

}

export class WeatherSensor extends VariableSensor {

  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
    }
  }

}

export class Alert extends Sensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      this.NotifyLink = [];
      this.data = item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
      if (this.data.indexOf('<br />') !== -1) {
        this.hasNewLine = true;
      }
      this.status = this.data;
      this.data = '';
      if (typeof item.Level !== 'undefined') {
        this.image = 'images/Alert48_' + item.Level + '.png';
      }
    }
  }
}


export class Baro extends WeatherSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (this.name === 'Baro') {
        this.name = 'Barometer';
      }
      this.image = 'images/baro48.png';
      this.LogLink = ['/BaroLog', this.index];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      if (typeof item.Barometer !== 'undefined') {
        this.data = this.smallStatus = item.Barometer + ' hPa';
        if (typeof item.ForecastStr !== 'undefined') {
          this.status = item.Barometer + ' hPa, ' + thisService.translationService.t('Prediction') + ': '
            + thisService.translationService.t(item.ForecastStr);
        } else {
          this.status = item.Barometer + ' hPa';
        }
        if (typeof item.Altitude !== 'undefined') {
          this.status += ', Altitude: ' + item.Altitude + ' meter';
        }
      }
    }
  }
}

export class Switch extends Sensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);

      const bIsOff = ((this.status === 'Off') || (item.Status === 'Closed') || (item.Status === 'Locked'));

      if (item.CustomImage !== 0) {
        this.image = (bIsOff) ? 'images/' + item.Image + '48_Off.png' : 'images/' + item.Image + '48_On.png';
      } else {
        this.image = (bIsOff) ? 'images/' + item.TypeImg + '48_Off.png' : 'images/' + item.TypeImg + '48_On.png';
      }
      this.data = '';
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.showStatus = (DomoticzDevicesService.Device.showSwitchValues === true);
      this.imagetext = 'Activate switch';
      this.controlable = true;
      this.onClick = () => {
        thisService.lightSwitchesService.SwitchLight(this.index, ((this.status === 'Off') ? 'On' : 'Off'), this.protected)
          .subscribe(() => {
            DomoticzDevicesService.Device.switchFunction();
          });
      };
    }
  }
}

export class BinarySwitch extends Switch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.onClick = () => {
        thisService.lightSwitchesService.SwitchLight(this.index, ((this.status === 'Off') ? 'On' : 'Off'), this.protected).subscribe(() => {
          DomoticzDevicesService.Device.switchFunction();
        });
      };
    }
  }
}

export class BinarySensor extends Sensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
    }
  }
}

export class SecuritySensor extends BinarySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = (this.status === 'On') ? 'images/' + item.TypeImg + '48-on.png' : 'images/' + item.TypeImg + '48-off.png';
    }
  }
}

export class TemperatureSensor extends VariableSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/temp48.png';
      this.status = thisService.translationService.t('Temp') + ': ' + this.data;
      this.LogLink = ['/Devices', this.index, 'Log'];
    }
  }
}

export class UtilitySensor extends VariableSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      // this.EditLink = "EditUtilityDevice(" + this.index + ",'" + this.name + "');";
    }
  }
}

export class Blinds extends Switch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.data = '';
      if (item.Status === 'Closed') {
        this.image = 'images/blinds48sel.png';
        this.image2 = 'images/blindsopen48.png';
      } else {
        this.image = 'images/blindsopen48sel.png';
        this.image2 = 'images/blinds48.png';
      }
      this.onClick = () => {
        thisService.lightSwitchesService.SwitchLight(this.index, ((item.SwitchType === 'Blinds Inverted') ? 'On' : 'Off'),
          this.protected).subscribe(() => {
            DomoticzDevicesService.Device.switchFunction();
          });
      };
      this.onClick2 = () => {
        thisService.lightSwitchesService.SwitchLight(this.index, ((item.SwitchType === 'Blinds Inverted') ? 'Off' : 'On'),
          this.protected).subscribe(() => {
            DomoticzDevicesService.Device.switchFunction();
          });
      };
      if (item.SwitchType === 'Blinds Percentage') {
        this.haveDimmer = true;
        this.image2 = '';
        this.onClick2 = null;
      }
    }
  }
}

export class Counter extends UtilitySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/counter.png';
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };

      if (typeof item.CounterToday !== 'undefined') {
        this.status += ' ' + thisService.translationService.t('Today') + ': ' + item.CounterToday;
        this.smallStatus = item.CounterToday;
      }
      if (typeof item.CounterDeliv !== 'undefined') {
        if (item.CounterDeliv !== 0) {
          if (item.UsageDeliv.charAt(0) !== 0) {
            this.status += '-' + item.UsageDeliv;
          }
          this.status += ', ' + thisService.translationService.t('Return') + ': ' + item.CounterDelivToday;
        }
      }
    }
  }
}

export class Contact extends BinarySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = (this.status === 'Closed') ? 'images/' + item.Image + '48_Off.png' : 'images/' + item.Image + '48_On.png';
      this.data = '';
      this.smallStatus = this.status;
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
    }
  }
}



export class Current extends UtilitySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.status = '';
      if (typeof item.Usage !== 'undefined') {
        this.status = (item.Usage !== this.data) ? item.Usage : '';
      }
      if (typeof item.CounterToday !== 'undefined') {
        this.status += ' ' + thisService.translationService.t('Today') + ': ' + item.CounterToday;
      }
      if (typeof item.CounterDeliv !== 'undefined') {
        if (item.CounterDeliv !== 0) {
          if (item.UsageDeliv.charAt(0) !== 0) {
            this.status += '-' + item.UsageDeliv;
          }
          this.status += ', ' + thisService.translationService.t('Return') + ': ' + item.CounterDelivToday;
        }
      }
      switch (this.type) {
        case 'Energy':
          this.LogLink = ['/Devices', this.index, 'Log'];
          this.onClick = () => {
            thisService.router.navigate(this.LogLink);
          };
          this.smallStatus = this.data;
          break;
        case 'Usage':
          this.LogLink = ['/Devices', this.index, 'Log'];
          this.onClick = () => {
            thisService.router.navigate(this.LogLink);
          };
          break;
        case 'General':
          switch (this.subtype) {
            case 'kWh':
              this.LogLink = ['/Devices', this.index, 'Log'];
              this.onClick = () => {
                thisService.router.navigate(this.LogLink);
              };
              this.smallStatus = this.data;
              break;
            case 'Voltage':
              this.LogLink = ['/Devices', this.index, 'Log'];
              this.onClick = () => {
                thisService.router.navigate(this.LogLink);
              };
              this.smallStatus = this.data;
              break;
            default:
              this.LogLink = ['/CurrentLog', this.index];
              this.onClick = () => {
                thisService.router.navigate(this.LogLink);
              };
              break;
          }
          break;
        default:
          this.LogLink = ['/CurrentLog', this.index];
          this.onClick = () => {
            thisService.router.navigate(this.LogLink);
          };
          break;
      }
      this.smallStatus = this.smallStatus.split(', ')[0];
    }
  }
}

export class Custom extends UtilitySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (item.CustomImage !== 0) {
        this.image = 'images/' + item.Image + '48_On.png';
      } else {
        this.image = 'images/Custom.png';
      }
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      this.data = '';
    }
  }
}

export class Dimmer extends Switch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.haveDimmer = true;
      this.data = '';
      this.smallStatus = (this.status === 'Off') ? 'Off' : item.Level + '%';
      if (item.CustomImage !== 0) {
        this.image = (this.status === 'Off') ? 'images/' + item.Image + '48_Off.png' : 'images/' + item.Image + '48_On.png';
      } else {
        this.image = (this.status === 'Off') ? 'images/Dimmer48_Off.png' : 'images/Dimmer48_On.png';
      }
      this.status = this.thisService.deviceService.TranslateStatus(this.status);
    }
  }
}

export class Door extends BinarySwitch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (item.CustomImage === 0) {
        this.image = ((this.status === 'Locked') || (this.status === 'Closed')) ? 'images/' + item.Image + '48_Off.png' :
          this.image = 'images/' + item.Image + '48_On.png';
      }
      this.data = '';
      this.NotifyLink = [];
      this.onClick = null;

      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
    }
  }
}

export class DoorContact extends BinarySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (item.CustomImage === 0) {
        this.image = (this.status === 'Closed') ? 'images/' + item.Image + '48_Off.png' :
          this.image = 'images/' + item.Image + '48_On.png';
      }
      this.imagetext = '';
      this.NotifyLink = [];
      this.onClick = null;
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      this.data = '';
    }
  }
}

export class Doorbell extends BinarySwitch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/doorbell48.png';
      this.data = '';
    }
  }
}

export class DuskSensor extends BinarySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = (item.Status === 'On') ? 'images/uvdark.png' : this.image = 'images/uvsunny.png';
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      this.data = '';
    }
  }
}

export class Group extends Switch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/Push48_On.png';
      this.onClick = () => {
        thisService.sceneService.SwitchScene(this.index, 'On', this.protected).subscribe(() => {
          // Nothing
        });
      };
      this.image2 = 'images/Push48_Off.png';
      this.onClick2 = () => {
        thisService.sceneService.SwitchScene(this.index, 'Off', this.protected).subscribe(() => {
          // Nothing
        });
      };
      (this.status === 'Off') ? this.image2_opacity = 0.5 : this.image_opacity = 0.5;
      this.data = '';
      this.showStatus = (DomoticzDevicesService.Device.showSceneNames || DomoticzDevicesService.Device.showSwitchValues);
      this.smallStatus = (DomoticzDevicesService.Device.showSwitchValues === true) ? this.status : this.name;
    }
  }
}

export class Hardware extends UtilitySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);

      if (this.subtype === 'General' || this.subtype === 'Percentage') {
        this.LogLink = ['/Devices', this.index, 'Log'];
      } else {
        this.LogLink = ['/' + this.subtype + 'Log', this.index];
        this.onClick = () => {
          thisService.router.navigate(this.LogLink);
        };
      }

      if (item.CustomImage === 0) {
        switch (item.SubType.toLowerCase()) {
          case 'percentage':
            this.image = 'images/Percentage48.png';
            break;
        }
      }
    }
  }
}

export class Humidity extends WeatherSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/moisture48.png';
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      if (typeof item.Humidity !== 'undefined') {
        this.data = this.smallStatus = item.Humidity + '%';
        this.status = thisService.translationService.t('Humidity') + ': ' + item.Humidity + '%';
        if (typeof item.HumidityStatus !== 'undefined') {
          this.status += ', ' + thisService.translationService.t(item.HumidityStatus);
        }
        if (typeof item.DewPoint !== 'undefined') {
          this.status += ', ' + thisService.translationService.t('Dew Point') + ': ' +
            item.DewPoint + '\u00B0' + thisService.configService.config.TempSign;
        }
      }
    }
  }
}

export class Lightbulb extends BinarySwitch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      item.TypeImg = 'Light';
      super(thisService, item);
    }
  }
}

export class MediaPlayer extends BinarySwitch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      //        item.TypeImg = "Media";
      if (item.Status.length === 1) {
        item.Status = 'Off';
      }
      super(thisService, item);
      this.image2 = 'images/remote48.png';
      this.onClick2 = () => {
        thisService.ShowMediaRemote(this.name, this.index, undefined);
      };
      if (this.status === 'Off') {
        this.image2_opacity = 0.5;
      }
      this.imagetext = '';
      this.data = this.status;
      this.status = item.Data.replace(';', ', ');
      while (this.status.search(';') !== -1) {
        this.status = this.status.replace(';', ', ');
      }
    }
  }
}

export class Motion extends SecuritySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (item.CustomImage === 0) {
        this.image = (this.status === 'On') ? 'images/' + item.TypeImg + '48-on.png' : 'images/' + item.TypeImg + '48-off.png';
      }
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      this.data = '';
      this.smallStatus = this.status;
    }
  }
}

export class Pushon extends BinarySwitch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/' + item.Image + '48_On.png';
      this.onClick = () => {
        thisService.lightSwitchesService.SwitchLight(this.index, 'On', this.protected).subscribe(() => {
          DomoticzDevicesService.Device.switchFunction();
        });
      };
    }
  }
}

export class Pushoff extends BinarySwitch {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/' + item.Image + '48_Off.png';
      this.onClick = () => {
        thisService.lightSwitchesService.SwitchLight(this.index, 'Off', this.protected).subscribe(() => {
          DomoticzDevicesService.Device.switchFunction();
        });
      };
    }
  }
}

export class Radiation extends WeatherSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
    }
  }
}

export class Rain extends WeatherSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (typeof item.Rain !== 'undefined') {
        this.status = item.Rain;
        if ($.isNumeric(item.Rain)) {
          this.status += ' mm';
        }
        this.data = this.smallStatus = this.status;
        if (typeof item.RainRate !== 'undefined') {
          if (item.RainRate !== 0) {
            this.status += ', Rate: ' + item.RainRate + ' mm/h';
          }
        }
      }
    }
  }
}

export class Scene extends Pushon {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      item.Status = 'Off';
      super(thisService, item);
      this.onClick = () => {
        thisService.sceneService.SwitchScene(this.index, 'On', this.protected).subscribe(() => {
          DomoticzDevicesService.Device.backFunction();
        });
      };
      this.data = '';
      this.LogLink = [];
      this.showStatus = DomoticzDevicesService.Device.showSceneNames;
      this.smallStatus = this.name;
    }
  }
}

export class SetPoint extends TemperatureSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/override.png';
      if ($.isNumeric(this.data)) {
        this.data += '\u00B0' + thisService.configService.config.TempSign;
      }
      if ($.isNumeric(this.status)) {
        this.status += '\u00B0' + thisService.configService.config.TempSign;
      }
      const pattern = new RegExp('\\d\\s' + thisService.configService.config.TempSign + '\\b');
      while (this.data.search(pattern) > 0) {
        this.data = Utils.setCharAt(this.data, this.data.search(pattern) + 1, '\u00B0');
      }
      while (this.status.search(pattern) > 0) {
        this.status = Utils.setCharAt(this.status, this.status.search(pattern) + 1, '\u00B0');
      }
    }
  }
}

export class Siren extends BinarySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = ((item.Status === 'On') || (item.Status === 'Chime') || (item.Status === 'Group On') ||
        (item.Status === 'All On')) ? 'images/siren-on.png' : this.image = 'images/siren-off.png';
      this.onClick = null;
    }
  }
}

export class Smoke extends BinarySensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = ((item.Status === 'Panic') || (item.Status === 'On')) ? 'images/smoke48on.png' :
        this.image = 'images/smoke48off.png';
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
      this.data = '';
    }
  }
}

export class Temperature extends TemperatureSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      if (typeof item.Temp !== 'undefined') {
        let temp = item.Temp;
        this.image = 'images/' + this.thisService.deviceService.GetTemp48Item(temp);
        if ($.isNumeric(temp)) {
          temp += '\u00B0' + thisService.configService.config.TempSign;
        }
        this.smallStatus = this.data = temp;
      }
      const pattern = new RegExp('\\d\\s' + thisService.configService.config.TempSign + '\\b');
      while (this.status.search(pattern) > 0) {
        this.status = Utils.setCharAt(this.status, this.status.search(pattern) + 1, '\u00B0');
      }
    }
  }
}

export class Text extends Sensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.ignoreClick = true;
      this.imagetext = '';
      this.NotifyLink = this.LogLink = [];
      this.data = item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
      if (this.data.indexOf('<br />') !== -1) {
        this.hasNewLine = true;
      }
      this.onClick = null;
      this.status = this.data;
      this.data = '';
    }
  }
}

export class Visibility extends WeatherSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.LogLink = ['/Devices', this.index, 'Log'];
      this.onClick = () => {
        thisService.router.navigate(this.LogLink);
      };
    }
  }
}

export class Wind extends WeatherSensor {
  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);
      this.image = 'images/wind48.png';
      if (typeof item.DirectionStr !== 'undefined') {
        this.image = 'images/Wind' + item.DirectionStr + '.png';
        this.status = item.DirectionStr;
        this.data = item.DirectionStr;
        this.smallStatus = item.DirectionStr;
        if (typeof item.Direction !== 'undefined') {
          this.status = item.Direction + ' ' + item.DirectionStr;
        }
      }
      if (typeof item.Speed !== 'undefined') {
        this.status += ', ' + item.Speed + ' ' + thisService.configService.config.WindSign;
        this.data += ' / ' + item.Speed + ' ' + thisService.configService.config.WindSign;
        this.smallStatus = item.Speed + ' ' + thisService.configService.config.WindSign;
      }
      if (typeof item.Gust !== 'undefined') {
        this.status += ', ' + thisService.translationService.t('Gust') + ': ' + item.Gust + ' ' +
          thisService.configService.config.WindSign;
      }
      if (typeof item.Chill !== 'undefined') {
        this.status += ', ' + thisService.translationService.t('Chill') + ': ' + item.Chill + '\u00B0' +
          thisService.configService.config.TempSign;
      }
    }
  }
}



export class Selector extends Switch {

  levelNames: string[];
  // FIXME types
  levelInt: any;
  levelName: string;
  levelOffHidden: any;

  constructor(thisService: DomoticzDevicesService, item?: any) {
    if (arguments.length === 2) {
      super(thisService, item);

      // Selector attributes
      this.levelNames = Utils.b64DecodeUnicode(item.LevelNames).split('|');
      this.levelInt = item.LevelInt;
      this.levelName = this.levelNames[this.levelInt / 10];
      this.levelOffHidden = item.LevelOffHidden;

      // Device attributes
      this.data = this.levelName;
      this.status = '';
      this.smallStatus = this.status;
      if (item.CustomImage !== 0) {
        this.image = (this.levelName === 'Off') ? 'images/' + item.Image + '48_Off.png' : 'images/' + item.Image + '48_On.png';
      } else {
        this.image = (this.levelName === 'Off') ? 'images/' + item.TypeImg + '48_Off.png' : 'images/' + item.TypeImg + '48_On.png';
      }
      if ((this.levelName === 'Off') || (this.levelOffHidden === true)) {
        this.ignoreClick = true;
        this.onClick = () => {
          // Do Nothing
        };
      }
      this.useSVGtags = false;
    }
  }

  // Selector methods

  onRefreshEvent() {
    // Nothing to do yet because status is update correctly
  }

  onSelectorValueChange(idx, levelname, level) {
    // Send device command
    this.thisService.lightSwitchesService.SwitchSelectorLevel(idx, unescape(levelname), level, this.protected).subscribe(() => {
      this.onRefreshEvent();
    });
  }

  toggleSelectorList(selector$) {
    const deviceDetails$ = selector$.parents('.DeviceDetails'),
      list$ = selector$.find('.level-list');
    // Show/Hide underlying controls
    if ((list$.css('display') === 'none') || (list$.css('visibility') !== 'visible')) {
      deviceDetails$
        .find('#lastseen').hide().end()
        .find('#twisty').hide().end()
        .find('#detailsgroup')
        .find('rect').hide().end()
        .find('text').hide().end()
        .end();
    } else {
      deviceDetails$
        .find('#lastseen').show().end()
        .find('#twisty').show().end()
        .find('#detailsgroup')
        .find('rect').show().end()
        .find('text').show().end()
        .end();
    }
    list$.toggle();
  }

  onSelectorEvent(evt) {
    const type = evt.type,
      target = evt.target,
      target$ = $(target),
      selector$ = target$.parents('.selector'),
      targetTagName = target$.prop('tagName'),
      parent$ = target$.parent(),
      parentTagName = parent$.prop('tagName');
    let text$, idx, levelName, levelInt;
    // let deviceDetails$, list$;
    if (type === 'mouseover') {
      parent$.children().addClass('hover');
      return;
    }
    if (type === 'mouseout') {
      parent$.children().removeClass('hover');
      return;
    }
    if (type === 'click') {
      if (parent$.hasClass('current-level')) {
        this.toggleSelectorList(selector$);
        return;
      }
      if (parent$.hasClass('level')) {
        text$ = parent$.find('text');
        levelName = text$.text();
        levelInt = parseInt(text$.data('value'), 10);
        idx = selector$.data('idx');
        selector$.find('.current-level text').text(levelName).data('value', levelInt).end();
        this.toggleSelectorList(selector$);
        this.onSelectorValueChange(idx, levelName, levelInt);
        return;
      }
      return;
    }
  }

  drawCustomStatus(elStatus) {
    const idx = this.index,
      levelInt = this.levelInt,
      levelName = this.levelName,
      levelNames = this.levelNames;
    let elSVG$, html;
    let elSelector, elCurrentLevel, elLevelList, elBack, elText, elArrow, elLevel, i;
    const selectorLeft = 0, selectorTop = 2, selectorWidth = 203,
      p = function (x, y) {
        return x + ' ' + y + ' ';
      },
      rectangle = function (x, y, w, h, r1, r2, r3, r4) {
        let strPath = 'M' + p(x + r1, y); // A
        strPath += 'L' + p(x + w - r2, y) + 'Q' + p(x + w, y) + p(x + w, y + r2); // B
        strPath += 'L' + p(x + w, y + h - r3) + 'Q' + p(x + w, y + h) + p(x + w - r3, y + h); // C
        strPath += 'L' + p(x + r4, y + h) + 'Q' + p(x, y + h) + p(x, y + h - r4); // D
        strPath += 'L' + p(x, y + r1) + 'Q' + p(x, y) + p(x + r1, y); // A
        strPath += 'Z';
        return strPath;
      };
    elSVG$ = $(DomoticzDevicesService.getSVGnode());
    if (elSVG$.length === 1) {
      if (elSVG$.find('style.selector-style').length < 1) {
        html = [];
        html.push('<style type="text/css" class="selector-style">');
        html.push('.selector {');
        html.push('pointer-events: all;');
        html.push('}');
        html.push('.selector .background {');
        html.push('fill: #f5f5f5; stroke-width: 1; stroke: #cccccc;');
        html.push('}');
        html.push('.selector .background.hover, .selector .background:hover {');
        html.push('fill: #0078a3;');
        html.push('}');
        html.push('.selector .level-text {');
        html.push('font-size: 10pt; font-weight: bold; fill: #333333;');
        html.push('}');
        html.push('.selector .level-text.hover, .selector .level-text:hover {');
        html.push('fill: #ffffff;');
        html.push('}');
        html.push('.selector .level-list .level path {');
        html.push('stroke: #000000; stroke-width: 1px; fill: #000000;');
        html.push('}');
        html.push('.selector .level-list .level path.hover, .selector .level-list .level path:hover {');
        html.push('fill: #0078a3;');
        html.push('}');
        html.push('.selector .level-list .level text {');
        html.push('fill: #ffffff; font-weight: normal;');
        html.push('}');
        html.push('.selector .level-list .level text.hover, .selector .level-list .level text:hover {');
        html.push('font-weight: bold;');
        html.push('}');
        html.push('.selector .arrow-s {');
        html.push('fill: #cccccc;');
        html.push('}');
        html.push('.selector .arrow-s.hover, .selector .arrow-s:hover {');
        html.push('fill: #ffffff;');
        html.push('}');
        html.push('</style>');
        elSVG$.prepend(html.join(''));
      }
    }
    elCurrentLevel = DomoticzDevicesService.makeSVGnode('g', { 'class': 'current-level' }, '');
    elBack = DomoticzDevicesService.makeSVGnode('rect', {
      'class': 'background',
      'width': selectorWidth,
      'height': 20,
      'x': selectorLeft,
      'y': selectorTop,
      'rx': 6,
      'ry': 6
    }, '');
    elText = DomoticzDevicesService.makeSVGnode('text', {
      'class': 'level-text',
      'x': (selectorLeft + 5),
      'y': (selectorTop + 15),
      'text-anchor': 'start',
      'data-value': levelInt
    }, levelName);
    elArrow = DomoticzDevicesService.makeSVGnode('polygon', {
      'class': 'arrow-s',
      'points': (selectorLeft + selectorWidth - 14) + ',' + (selectorTop + 9) + ' ' + (selectorLeft + selectorWidth - 8) +
        ',' + (selectorTop + 9) + ' ' + (selectorLeft + selectorWidth - 11) + ',' + (selectorTop + 13)
    }, '');
    elCurrentLevel.appendChild(elBack);
    elCurrentLevel.appendChild(elText);
    elCurrentLevel.appendChild(elArrow);

    elLevelList = DomoticzDevicesService.makeSVGnode('g', { 'class': 'level-list', 'style': 'display: none;' }, '');
    i = 0;
    levelNames.forEach((text, index) => {
      if ((index === 0) && (this.levelOffHidden === true)) {
        return;
      }
      const height = 20, width = selectorWidth,
        x = selectorLeft, y = (selectorTop + 20) + (i * 20),
        r1 = 0, r2 = 0,
        r3 = (i === (levelNames.length - 1)) ? 6 : 0, r4 = r3;
      elLevel = DomoticzDevicesService.makeSVGnode('g', { 'class': 'level' }, '');
      elBack = DomoticzDevicesService.makeSVGnode('path', {
        'class': 'background',
        'd': rectangle(x, y, width, height, r1, r2, r3, r4)
      }, '');
      elText = DomoticzDevicesService.makeSVGnode('text', {
        'class': 'level-text',
        'data-value': (index * 10),
        'x': (x + 5),
        'y': (y + 15),
        'text-anchor': 'start'
      }, text);
      elLevel.appendChild(elBack);
      elLevel.appendChild(elText);
      elLevelList.appendChild(elLevel);
      i += 1;
    });
    elSelector = DomoticzDevicesService.makeSVGnode('g', { 'class': 'selector', 'data-idx': idx }, '');
    $(elSelector).bind('mouseover mouseout click', this.onSelectorEvent);
    elSelector.appendChild(elCurrentLevel);
    elSelector.appendChild(elLevelList);
    elStatus.appendChild(elSelector);
  }
}

export class Transform {

  scale = 1.0;
  rotation = 0;
  xRotation = 0;
  yRotation = 0;
  xOffset = 0;
  yOffset = 0;

  constructor(tag?: any) {
    if (arguments.length === 2) {
      let sTransform;
      if (typeof tag === 'string') {
        sTransform = tag;
      }
      if (typeof tag === 'object') {
        if ((tag instanceof SVGGElement) || (tag instanceof SVGImageElement)) {
          sTransform = tag.getAttribute('transform');
        }
        if (tag[0] instanceof SVGGElement) {
          sTransform = tag[0].getAttribute('transform');
        }
        if ((typeof sTransform !== 'undefined') && (sTransform != null) && (sTransform.length !== 0)) {
          const aTransform = sTransform.split(/[,() ]/);
          let iVal;
          for (let i = 0; i < aTransform.length; i++) {
            if (aTransform[i].toLowerCase() === 'translate') {
              iVal = Number(aTransform[++i]);
              if (!isNaN(iVal)) {
                this.xOffset = iVal;
              }
              iVal = Number(aTransform[++i]);
              if (!isNaN(iVal)) {
                this.yOffset = iVal;
              }
              continue;
            }
            if (aTransform[i].toLowerCase() === 'scale') {
              const fVal = parseFloat(aTransform[++i]);
              if (!isNaN(fVal)) {
                this.scale = fVal;
              }
              continue;
            }
            if (aTransform[i].toLowerCase() === 'rotate') {
              iVal = Number(aTransform[++i]);
              if (!isNaN(iVal)) {
                this.rotation = iVal;
              }
              iVal = Number(aTransform[++i]);
              if (!isNaN(iVal)) {
                this.xRotation = iVal;
              }
              iVal = Number(aTransform[++i]);
              if (!isNaN(iVal)) {
                this.yRotation = iVal;
              }
            }
          }
        }
      }
    }
  }


  Add(tag) {
    if (tag instanceof Transform) {
      this.scale *= tag.scale;
      this.xOffset += tag.xOffset;
      this.yOffset += tag.yOffset;
      this.rotation += tag.rotation;
      this.xRotation += tag.xRotation;
      this.yRotation += tag.yRotation;
    }
  }

  Calculate(oTarget) {
    try {
      const svg = DomoticzDevicesService.getSVGnode();
      const oMatrix = oTarget.getTransformToElement(svg);  // sometimes throws errors on IE 11
      this.scale = (oMatrix.d === 1) ? 0 : oMatrix.d;
      this.xOffset = oMatrix.e;
      this.yOffset = oMatrix.f;
    } catch (err) {
      let oTemp = oTarget.parentNode;
      const oTransform = new Transform();
      while (oTemp.nodeName.toLowerCase() !== 'svg') {
        const oNew = new Transform(oTemp);
        oTransform.Add(oNew);
        oTemp = oTemp.parentNode;
      }
      this.scale = oTransform.scale;
      this.xOffset = oTransform.xOffset;
      this.yOffset = oTransform.yOffset;
    }
  }

  toString() {
    let retVal = '';
    if (this.scale !== 1.0) {
      retVal += 'scale(' + this.scale + ')';
    }
    if ((this.xOffset + this.yOffset) !== 0) {
      retVal += ' translate(' + this.xOffset + ',' + this.yOffset + ')';
    }
    if ((this.rotation + this.xRotation + this.yRotation) !== 0) {
      retVal += ' rotate(' + this.rotation + ',' + this.xRotation + ',' + this.yRotation + ')';
    }
    return retVal;
  }
}

export class Slider {

  curLevel = 0;
  newLevel = 0;
  maxLevel = 0;
  oSliderHandle;

  constructor(private thisService: DomoticzDevicesService, e?: any) {
    if (arguments.length === 1) {
      $('.SliderBack')
        .off()
        .bind('click', function (event, ui) {
          const oSlider = new Slider(thisService, event);
          oSlider.Click(event);
        });
      $('.Slider')
        .off()
        .bind('click', function (event, ui) {
          const oSlider = new Slider(thisService, event);
          oSlider.Click(event);
        });
      $('.SliderHandle')
        .off()
        .draggable()
        .on('drag', function (event, ui) {
          const oSlider = new Slider(thisService, event);
          oSlider.Drag(event);
        })
        .on('dragstop', function (event, ui) {
          const oSlider = new Slider(thisService, event);
          oSlider.Click(event);
          $('.SliderHandle').draggable();
        });
    }
  }

  Initialise(event) {
    if (typeof (event.clientX) === 'undefined') {
      // $.cachenoty=generate_noty('error', '<b>event clientx is '+event.originalEvent.clientX+'</b>', 250);
      return;
    }
    let svg = event.target;
    while ((svg.tagName !== 'svg') && (svg.tagName !== 'body')) {
      svg = svg.parentNode;
      if (svg == null) {
        return;
      }
    }
    if (svg.tagName !== 'svg') {
      return;
    }

    const oGroup = event.target.parentNode;
    let oSliderBack;
    let oSlider;
    for (let i = 0; i < oGroup.childNodes.length; i++) {
      if (oGroup.childNodes[i].id === 'sliderback') {
        oSliderBack = oGroup.childNodes[i];
      }
      if (oGroup.childNodes[i].id === 'slider') {
        oSlider = oGroup.childNodes[i];
      }
      if (oGroup.childNodes[i].id === 'sliderhandle') {
        this.oSliderHandle = oGroup.childNodes[i];
      }
    }

    const pt = svg.createSVGPoint();
    pt.x = oSliderBack.getAttribute('x');
    pt.y = oSliderBack.getAttribute('y');
    const backWidth = oSliderBack.getAttribute('width');
    const matrix = oSliderBack.getScreenCTM();
    const transformed = pt.matrixTransform(matrix);
    let result = (event.clientX - transformed.x) / (backWidth * matrix.a);   // should give a percentage
    if (result < 0) {
      result = 0;
    }
    if (result > 1) {
      result = 1;
    }
    this.curLevel = Number(this.oSliderHandle.getAttribute('level'));
    this.maxLevel = Number(this.oSliderHandle.getAttribute('maxlevel'));
    this.newLevel = Math.round(this.maxLevel * result);
    const handleWidth = Number(this.oSliderHandle.getAttribute('width'));
    this.oSliderHandle.setAttribute('x', (backWidth * (this.newLevel / this.maxLevel)) - (handleWidth / 2));
    oSlider.setAttribute('width', (backWidth * (this.newLevel / this.maxLevel)));
  }

  Click(event) {
    this.Initialise(event);
    if (this.newLevel !== this.curLevel) {
      this.oSliderHandle.setAttribute('level', this.newLevel);
      const idx = Number(this.oSliderHandle.getAttribute('devindex'));
      // SetDimValue(idx, newLevel);  // this is synchronous and makes this quite slow
      if (this.thisService.permissionService.getRights() === 0) {
        this.thisService.notificationService.HideNotify();
        this.thisService.notificationService.ShowNotify(
          this.thisService.translationService.t('You do not have permission to do that!'), 2500, true);
        return;
      }
      this.thisService.apiService.callApi('command', {
        param: 'switchlight',
        idx: idx.toString(),
        switchcmd: 'Set Level',
        level: this.newLevel.toString()
      }).subscribe(() => {
        // Nothing
      });
      timer(250).subscribe(() => {
        DomoticzDevicesService.Device.switchFunction();
      });
    }
  }

  Drag(event) {
    this.Initialise(event);
  }
}
