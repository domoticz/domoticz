import {AfterViewInit, Component, Inject, OnDestroy, OnInit} from '@angular/core';
import {NotyHelper} from 'src/app/_shared/_utils/noty-helper';
import {FloorplansService} from 'src/app/_shared/_services/floorplans.service';
import {Floorplan} from 'src/app/_shared/_models/floorplans';
import {ConfigService} from '../../_shared/_services/config.service';
import {Subscription, timer} from 'rxjs';
import {DeviceService} from '../../_shared/_services/device.service';
import {DomoticzDevicesService, Transform} from '../../_shared/_services/domoticz-devices.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {LivesocketService} from '../../_shared/_services/livesocket.service';
import {TimesunService} from '../../_shared/_services/timesun.service';
import {Device, DevicesResponse} from '../../_shared/_models/device';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-floorplans',
  templateUrl: './floorplans.component.html',
  styleUrls: ['./floorplans.component.css']
})
export class FloorplansComponent implements OnInit, AfterViewInit, OnDestroy {

  debug = 0;
  floorPlans: Array<FloorplanEnriched> = [];
  FloorplanCount: number;
  actFloorplan: number;
  browser = 'unknown';
  lastUpdateTime = 0;
  isScrolling = false;	// used on tablets & phones
  pendingScroll = false;	// used on tablets & phones
  lastTouch = 0;			// used on tablets & phones

  mytimer: Subscription;

  constructor(
    private floorplansService: FloorplansService,
    private configService: ConfigService,
    private deviceService: DeviceService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private domoticzDevicesService: DomoticzDevicesService,
    private livesocketService: LivesocketService,
    private timesunService: TimesunService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    try {
      this.ShowFloorplans();

      this.mytimer = this.livesocketService.device_update.subscribe(deviceData => {
        this.RefreshItem(deviceData);
      });

      document.getElementById('cFloorplans').addEventListener('mouseover', this.debugOn);
      document.getElementById('cFloorplans').addEventListener('mouseout', this.debugOff);
    } catch (err) {
      NotyHelper.generate_noty('error', '<b>Error Initialising Page</b><br>' + err, false);
    }
  }

  ngOnDestroy() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }
    const fp = document.getElementById('cFloorplans');
    if (fp) {
      fp.removeEventListener('mouseover', this.debugOn);
      fp.removeEventListener('mouseout', this.debugOff);
    }
    $('body').trigger('pageexit');
  }

  private ShowFloorplans() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }

    DomoticzDevicesService.Device.useSVGtags = true;
    DomoticzDevicesService.Device.backFunction = () => {
      this.ShowFloorplans();
    };
    DomoticzDevicesService.Device.switchFunction = () => {
      this.RefreshFPDevices();
    };
    DomoticzDevicesService.Device.contentTag = 'floorplancontent';
    DomoticzDevicesService.Device.xImageSize = 1280;
    DomoticzDevicesService.Device.yImageSize = 720;
    DomoticzDevicesService.Device.checkDefs();

    this.floorplansService.getFloorplans().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        if (typeof this.actFloorplan === 'undefined') {
          this.FloorplanCount = data.result.length;
          this.floorPlans = data.result;
          this.actFloorplan = 0;
          this.browser = 'unknown';
        } else {
          if (this.debug > 0) {
            this.configService.cachenoty =
              NotyHelper.generate_noty('info', '<b>Floorplan already set to: ' + this.actFloorplan + '</b>', 5000);
          }
        }

        // handle settings
        if (typeof data.AnimateZoom !== 'undefined') {
          this.configService.globals.AnimateTransitions = (data.AnimateZoom !== 0);
        }
        if (typeof data.FullscreenMode !== 'undefined') {
          this.configService.globals.FullscreenMode = (data.FullscreenMode !== 0);
        }
        if (typeof data.RoomColour !== 'undefined') {
          this.configService.globals.RoomColour = data.RoomColour;
        }
        if (typeof data.ActiveRoomOpacity !== 'undefined') {
          this.configService.globals.ActiveRoomOpacity = data.ActiveRoomOpacity;
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

        // Lets start
        data.result.forEach((item: Floorplan, i: number) => {
          const tagName = item.Name.replace(/\s/g, '_') + '_' + item.idx;
          this.floorPlans[i].floorID = item.idx;
          this.floorPlans[i].xImageSize = 0;
          this.floorPlans[i].yImageSize = 0;
          this.floorPlans[i].ScaleFactor = item.ScaleFactor;
          this.floorPlans[i].Name = item.Name;
          this.floorPlans[i].tagName = tagName;
          this.floorPlans[i].Image = item.Image;
          this.floorPlans[i].Loaded = false;
        });

        this.RefreshFPDevices();
      }

      const _this = this;

      if ((typeof this.configService.globals.RoomColour !== 'undefined') &&
        (typeof this.configService.globals.InactiveRoomOpacity !== 'undefined')) {
        $('.hoverable').css({
          'fill': this.configService.globals.RoomColour,
          'fill-opacity': this.configService.globals.InactiveRoomOpacity / 100
        });
      }
      if (typeof this.configService.globals.ActiveRoomOpacity !== 'undefined') {
        $('.hoverable')
          .hover(
            function () {
              $(this).css({'fill-opacity': _this.configService.globals.ActiveRoomOpacity / 100});
            },
            function () {
              $(this).css({'fill-opacity': _this.configService.globals.InactiveRoomOpacity / 100});
            });
      }

      // Hide menus if query string contains 'fullscreen'
      if (window.location.href.search('fullscreen') > 0) {
        this.doubleClick();
      }

      $('#fpwrapper')
        .dblclick(() => {
          _this.doubleClick();
        })
        .attr('fullscreen', ($('.navbar').css('display') === 'none') ? 'true' : 'false');

      // FIXME replace with a HostListener?
      $(window).resize(() => {
        _this.FloorplanResize();
      });

      document.addEventListener('touchstart', (ev) => _this.FPtouchstart(ev), false);
      document.addEventListener('touchmove', (ev) => _this.FPtouchmove(ev), false);
      document.addEventListener('touchend', (ev) => _this.FPtouchend(ev), false);
      $('body').css('overflow', 'hidden')
        .on('pageexit', function () {
          document.removeEventListener('touchstart', (ev) => _this.FPtouchstart(ev));
          document.removeEventListener('touchmove', (ev) => _this.FPtouchmove(ev));
          document.removeEventListener('touchend', (ev) => _this.FPtouchend(ev));
          $(window).off('resize');
          $('body').off('pageexit').css('overflow', '');

          // Make vertical scrollbar disappear
          $('#fpwrapper').attr('style', 'display:none;');
          $('#floorplancontent').addClass('container ng-scope').attr('style', '');

          // Move nav bar with Back and Report button down
          if ($('.navbar').css('display') !== 'none') {
            $('#floorplancontent').offset({top: $('.navbar').height()});
          } else {
            $('#floorplancontent').offset({top: 0});
          }
          $('#copyright').attr('style', 'position:absolute');
          if (_this.debug > 0) {
            _this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>PageExit code executed</b>', 2000);
          }
        });

    });
  }

  onBulletCellClick(related: string) {
    $('#BulletImages').children().css({'display': 'none'});
    this.ScrollFloorplans(related);
  }

  onBulletCellEnter(related: string) {
    $('#bulletcellchild' + related).css({'background': this.configService.globals.RoomColour});
    $('#' + related + '_bullet').css({'display': 'inline'});
  }

  onBulletCellLeave(related: string) {
    $('#bulletcellchild' + related).css({'background': ''});
    $('#' + related + '_bullet').css({'display': 'none'});
  }

  private FPtouchstart(e) {
    this.isScrolling = false;
  }

  private FPtouchmove(e) {
    this.isScrolling = true;
  }

  private FPtouchend(e) {
    //  Handle events on navigation elements
    if (e.target.getAttribute('related') != null) {
      $('#BulletImages').children().css({'display': 'none'});
      if (this.debug > 0) {
        this.configService.cachenoty =
          NotyHelper.generate_noty('info', '<b>Scrolling to: ' + e.target.getAttribute('related') + '</b>', 1000);
      }
      e.preventDefault();
      this.ScrollFloorplans(e.target.getAttribute('related'));
    } else {
      // otherwise do scrolling stuff
      if (this.isScrolling === true) {
        this.isScrolling = false;
        this.pendingScroll = true;
        timer(50).subscribe(() => {
          if ((this.isScrolling === false) && (this.pendingScroll === true)) {
            if (this.debug > 0) {
              this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Scrolled to: ' + window.pageXOffset + '</b>', 1000);
            }
            this.pendingScroll = false;
            let nearestFP = $('.imageparent:first');
            $('.imageparent').each(function () {
              const offset = Math.abs(window.pageXOffset - $(this).offset().left);
              if (offset < Math.abs(window.pageXOffset - nearestFP.offset().left)) {
                nearestFP = $(this);
              }
            });
            if (this.debug > 0) {
              this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Closest is: ' + nearestFP.attr('id') + '</b>', 1000);
            }
            this.ScrollFloorplans(nearestFP.attr('id'), true);
          }
        });
      } else {
        // if not scrolling look for double tap
        const delta = (new Date()).getTime() - this.lastTouch;
        const delay = 500;
        if (this.debug > 0) {
          this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Tap Delta: ' + delta + '</b>', 1000);
        }
        if (delta < delay && delta > 0) {
          this.doubleClick();
        }
        this.lastTouch = (new Date()).getTime();
      }
    }
  }

  private doubleClick() {
    if (this.configService.globals.FullscreenMode === true) {
      if ($('#fpwrapper').attr('fullscreen') !== 'true') {
        if (this.debug > 1) {
          this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Double Click -> Fullscreen!!</b>', 1000);
        }
        $('#copyright').css({display: 'none'});
        $('.navbar').css({display: 'none'});
        $('#fpwrapper').css({position: 'absolute', top: 0, left: 0}).attr('fullscreen', 'true');
      } else {
        if (this.debug > 1) {
          this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Double Click <- Fullscreen!!</b>', 1000);
        }
        $('#copyright').css({display: 'block'});
        $('.navbar').css({display: 'block'});
        $('#fpwrapper').css({position: 'relative'}).attr('fullscreen', 'false');
      }
      this.FloorplanResize();
    } else {
      this.configService.cachenoty = NotyHelper.generate_noty('warning', '<b>' +
        this.translationService.t('Fullscreen mode is disabled') + '</b>', 3000);
    }
  }

  ImgLoaded(tagName: string) {
    const target: any = document.getElementById(tagName + '_img');
    if (target != null) {
      for (let i = 0; i < this.floorPlans.length; i++) {
        if (this.floorPlans[i].idx === target.parentNode.getAttribute('index')) {
          this.floorPlans[i].xImageSize = target.naturalWidth;
          this.floorPlans[i].yImageSize = target.naturalHeight;
          this.floorPlans[i].Loaded = true;
          // Image has loaded, now load it into SVG
          const svgCont = document.getElementById(tagName + '_svg');
          svgCont.setAttribute('viewBox', '0 0 ' + target.naturalWidth + ' ' + target.naturalHeight);
          svgCont.appendChild(DomoticzDevicesService.makeSVGnode('g', {
            id: tagName + '_grp',
            transform: 'translate(0,0) scale(1)',
            style: '',
            zoomed: 'false'
          }, ''));
          // svgCont.childNodes[0].appendChild(DomoticzDevicesService.makeSVGnode('rect', {
          //   width: '100%',
          //   height: '100%',
          //   fill: 'white'
          // }, ''));
          svgCont.childNodes[0].appendChild(DomoticzDevicesService.makeSVGnode('image', {
            width: '100%',
            height: '100%',
            'xlink:href': this.floorPlans[i].Image
          }, ''));
          svgCont.childNodes[0].appendChild(DomoticzDevicesService.makeSVGnode('g', {
            id: tagName + '_Content',
            'class': 'FloorContent'
          }, ''));
          svgCont.childNodes[0].appendChild(DomoticzDevicesService.makeSVGnode('g', {
            id: tagName + '_Rooms',
            'class': 'FloorRooms'
          }, ''));
          svgCont.childNodes[0].appendChild(DomoticzDevicesService.makeSVGnode('g', {
            id: tagName + '_Devices',
            transform: 'scale(1)'
          }, ''));
          svgCont.setAttribute('style', 'display:inline; position: relative;');
          target.parentNode.removeChild(target);
          this.ShowRooms(i);
          break;
        }
      }
      // kick off periodic refresh once all images have loaded
      let AllLoaded = true;
      for (let i = 0, len = this.floorPlans.length; i < len; i++) {
        if (this.floorPlans[i].Loaded !== true) {
          AllLoaded = false;
          break;
        }
      }
      const _this = this;
      if (AllLoaded === true) {
        DomoticzDevicesService.Device.checkDefs();
        $('.FloorRooms').click((event) => {
          _this.RoomClick(event);
        });
        this.RefreshFPDevices();
        this.FloorplanResize();
      }
    } else {
      NotyHelper.generate_noty('error', '<b>ImageLoaded Error</b><br>Element not found: ' + tagName, false);
    }
  }

  GetFloorplanIdx(floorID: string): number {
    for (let i = 0; i < this.floorPlans.length; i++) {
      if (this.floorPlans[i].floorID === floorID) {
        return i;
      }
    }
    return -1;
  }

  private RoomClick(click) {
    $('.DeviceDetails').css('display', 'none');   // hide all popups
    const borderRect = click.target.getBBox(); // polygon bounding box
    const margin = 0.1;  // 10% margin around polygon
    const marginX = borderRect.width * margin;
    const marginY = borderRect.height * margin;
    const scaleX = this.floorPlans[this.actFloorplan].xImageSize / (borderRect.width + (marginX * 2));
    const scaleY = this.floorPlans[this.actFloorplan].yImageSize / (borderRect.height + (marginY * 2));
    const scale = ((scaleX > scaleY) ? scaleY : scaleX);

    const _this = this;

    const svgFloor = click.target.parentNode.parentNode.getAttribute('id');
    if ($('#' + svgFloor).attr('zoomed') === 'true') {
      if (this.configService.globals.AnimateTransitions) {
        $('#' + svgFloor).css('paddingTop', (borderRect.y - marginY))
          .css('paddingLeft', (borderRect.x - marginX))
          .css('paddingRight', scale)
          .animate({
              paddingTop: 0,
              paddingLeft: 0,
              paddingRight: 1.0
            },
            {
              duration: 300,
              progress: (animation, progress, remainingMs) => {
                _this.animateMove(svgFloor);
              },
              always: () => {
                $('#' + svgFloor).attr('zoomed', 'false')
                  .css('paddingTop', '').css('paddingLeft', '').css('paddingRight', '')
                  .attr('transform', 'translate(0,0) scale(1)');
              }
            });
      } else {
        $('#' + svgFloor).attr('zoomed', 'false')
          .css('paddingTop', '').css('paddingLeft', '').css('paddingRight', '')
          .attr('transform', 'translate(0,0) scale(1)');
      }
    } else {
      const attr = 'scale(' + scale + ')' + ' translate(' + (borderRect.x - marginX) * -1 + ',' + (borderRect.y - marginY) * -1 + ')';

      if (this.configService.globals.AnimateTransitions) {
        $('#' + svgFloor).css('paddingTop', 0).css('paddingLeft', 0).css('paddingRight', 1)
          .animate({
              paddingTop: (borderRect.y - marginY),
              paddingLeft: (borderRect.x - marginX),
              paddingRight: scale
            },
            {
              duration: 300,
              progress: (animation, progress, remainingMs) => {
                _this.animateMove(svgFloor);
              },
              always: () => {
                $('#' + svgFloor).attr('zoomed', 'true')
                  .css('paddingTop', '').css('paddingLeft', '').css('paddingRight', '')
                  .attr('transform', attr);
              }
            });
      } else {
        // this actually needs to centre in the direction its not scaling but close enough for v1.0
        $('#' + svgFloor)[0].setAttribute('transform', attr);
        $('#' + svgFloor).attr('zoomed', 'true');
      }
    }
  }

  private animateMove(svgFloor) {
    const element = $('#' + svgFloor)[0];
    const oTransform = new Transform(element);
    if (element.style.paddingLeft !== '') {
      oTransform.xOffset = Number(element.style.paddingLeft.replace('px', '').replace(';', '')) * -1;
    }
    if (element.style.paddingTop !== '') {
      oTransform.yOffset = Number(element.style.paddingTop.replace('px', '').replace(';', '')) * -1;
    }
    if (element.style.paddingRight !== '') {
      oTransform.scale = parseFloat(element.style.paddingRight.replace('px', '').replace(';', ''));
    }
    $('#' + svgFloor)[0].setAttribute('transform', oTransform.toString());
  }

  private FloorplanResize() {
    if (typeof $('#floorplancontent') !== 'undefined') {
      let wrpHeight = window.innerHeight;
      // when the small menu bar is displayed main-view jumps to the top so force it down
      if ($('.navbar').css('display') !== 'none') {
        $('#floorplancontent').offset({top: $('.navbar').height()});
        wrpHeight = window.innerHeight - $('#floorplancontent').offset().top -
          (($('#copyright').css('display') === 'none') ? 0 : $('#copyright').height()) - 52;
      } else {
        $('#floorplancontent').offset({top: 0});
      }
      $('#floorplancontent').width($('#main-view').width()).height(wrpHeight);
      if (this.debug > 0) {
        this.configService.cachenoty = NotyHelper.generate_noty('info',
          '<b>Window: ' + window.innerWidth + 'x' + window.innerHeight + '</b><br/><b>View: ' +
          $('#floorplancontent').width() + 'x' + wrpHeight + '</b>', 1000);
      }
      $('.imageparent').each(function (i) {
        $('#' + $(this).attr('id') + '_svg').width($('#floorplancontent').width()).height(wrpHeight);
      });
      if (this.FloorplanCount > 1) {
        $('#BulletGroup:first').css('left', (window.innerWidth - $('#BulletGroup:first').width()) / 2)
          .css('bottom', ($('#copyright').css('display') === 'none') ? 10 : $('#copyright').height() + 10)
          .css('display', 'inline');
      }
      if (typeof this.actFloorplan !== 'undefined') {
        this.ScrollFloorplans(this.floorPlans[this.actFloorplan].tagName, false);
      }
    }
  }

  private ScrollFloorplans(tagName: string, animate?: boolean) {
    if (this.debug > 0) {
      this.configService.cachenoty = NotyHelper.generate_noty('info', '<b>Scrolling to: ' + tagName + '</b>', 1000);
    }
    let allowAnimation = this.configService.globals.AnimateTransitions;
    if (arguments.length > 1) {
      allowAnimation = animate;
    }
    const target = document.getElementById(tagName);
    if (target != null) {
      for (let i = 0, len = this.floorPlans.length; i < len; i++) {
        if (this.floorPlans[i].idx === target.getAttribute('index')) {
          if (allowAnimation === false) {
            window.scrollTo($('#floorplancontent').width() * i, 0);
          } else {
            const from = {property: window.pageXOffset};  // starting position is current scrolled amount
            const to = {property: $('#floorplancontent').width() * i};
            $(from).animate(to, {
              duration: 500, easing: 'easeOutQuint', step: function (val) {
                window.scrollTo(val, 0);
              }
            });
          }
          $('.bulletSelected').attr('class', 'bullet');
          const bullet = $('.bullet')[i];
          if (bullet !== undefined) {
            bullet.setAttribute('class', 'bulletSelected');
          }
          this.actFloorplan = i;
          break;
        }
      }
    }
    $('#copyright').attr('style', 'position:fixed');
  }

  private RefreshItem(item: Device) {
    const _this = this;
    const itemCopy: any = Object.assign({}, item);
    const compoundDevice = (itemCopy.Type.indexOf('+') >= 0);
    $('.Device_' + itemCopy.idx).each(function () {
      const aSplit = $(this).attr('id').split('_');
      itemCopy.FloorID = aSplit[1];
      itemCopy.PlanID = aSplit[2];
      const floorPlan = _this.floorPlans[_this.GetFloorplanIdx(itemCopy.FloorID)];
      itemCopy.Scale = floorPlan ? floorPlan.ScaleFactor : undefined;
      if (compoundDevice) {
        itemCopy.Type = aSplit[0];					// handle multi value sensors (Baro, Humidity, Temp etc)
        itemCopy.CustomImage = 1;
        itemCopy.Image = aSplit[0].toLowerCase();
      }
      try {
        const dev = DomoticzDevicesService.Device.create(itemCopy);
        const existing = document.getElementById(dev.uniquename);
        if (existing !== undefined) {
          if (_this.debug > 2) {
            _this.configService.cachenoty = NotyHelper.generate_noty('info',
              '<b>Refreshing Device ' + dev.name + ((compoundDevice) ? ' - ' + itemCopy.Type : '') + '</b>', 2000);
          }
          dev.htmlMinimum(existing.parentNode);
        }
      } catch (err) {
        _this.configService.cachenoty = NotyHelper.generate_noty('error',
          '<b>Device refresh error</b> ' /*+ dev.name*/ + '<br>' + err, 5000);
      }
    });
  }

  private RefreshFPDevices() {
    if (typeof this.mytimer !== 'undefined') {
      this.mytimer.unsubscribe();
      this.mytimer = undefined;
    }

    this.livesocketService.getJson<DevicesResponse>('json.htm?type=devices&filter=all&used=true&order=Name&lastupdate=' +
      this.lastUpdateTime, (data) => {
      if (typeof data.ServerTime !== 'undefined') {
        this.timesunService.SetTimeAndSun(data);
      }

      /*
            Render all the widgets at once.
          */
      if (typeof data.result !== 'undefined') {
        if (typeof data.ActTime !== 'undefined') {
          this.lastUpdateTime = data.ActTime;
        }

        data.result.forEach((item: Device, i: number) => {
          this.RefreshItem(item);
        });
      }
    });
  }

  private ShowRooms(floorIdx: number) {
    this.floorplansService.getFloorplan(this.floorPlans[floorIdx].idx).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        data.result.forEach((item, i) => {
          $('#' + this.floorPlans[floorIdx].tagName + '_Rooms')
            .append(DomoticzDevicesService.makeSVGnode('polygon', {
              id: item.Name,
              'class': 'hoverable',
              points: item.Area
            }, item.Name));
        });
        if ((typeof this.configService.globals.RoomColour !== 'undefined') &&
          (typeof this.configService.globals.InactiveRoomOpacity !== 'undefined')) {
          $('.hoverable').css({
            'fill': this.configService.globals.RoomColour,
            'fill-opacity': this.configService.globals.InactiveRoomOpacity / 100
          });
        }
        if (typeof this.configService.globals.ActiveRoomOpacity !== 'undefined') {
          const _this = this;
          $('.hoverable').hover(function () {
            $(this).css({'fill-opacity': _this.configService.globals.ActiveRoomOpacity / 100});
          }, function () {
            $(this).css({'fill-opacity': _this.configService.globals.InactiveRoomOpacity / 100});
          });
        }
      }
    });

    this.ShowFPDevices(floorIdx);
  }

  private ShowFPDevices(floorIdx: number) {
    this.deviceService.getDevices('all', true, 'Name', undefined, undefined, undefined, this.floorPlans[floorIdx].idx).subscribe(data => {
      if ((typeof data.ActTime !== 'undefined') && (this.lastUpdateTime === 0)) {
        this.lastUpdateTime = data.ActTime;
      }
      // insert devices into the document
      let dev;
      if (typeof data.result !== 'undefined') {
        const elDevices = document.getElementById(this.floorPlans[floorIdx].tagName + '_Devices');
        const elIcons = DomoticzDevicesService.makeSVGnode('g', {id: 'DeviceIcons'}, '');
        elDevices.appendChild(elIcons);
        const elDetails = DomoticzDevicesService.makeSVGnode('g', {id: 'DeviceDetails'}, '');
        elDevices.appendChild(elDetails);
        data.result.forEach((item, i) => {
          const itemCopy: any = Object.assign({}, item);
          itemCopy.Scale = this.floorPlans[floorIdx].ScaleFactor;
          itemCopy.FloorID = this.floorPlans[floorIdx].floorID;
          if (itemCopy.Type.indexOf('+') >= 0) {
            const aDev = itemCopy.Type.split('+');
            let k;
            for (k = 0; k < aDev.length; k++) {
              const sDev = aDev[k].trim();
              itemCopy.Name = ((k === 0) ? itemCopy.Name : sDev);
              itemCopy.Type = sDev;
              itemCopy.CustomImage = 1;
              itemCopy.Image = sDev.toLowerCase();
              itemCopy.XOffset = Math.abs(itemCopy.XOffset) + ((k === 0) ? 0 : (50 * Number(this.floorPlans[floorIdx].ScaleFactor)));
              dev = DomoticzDevicesService.Device.create(itemCopy);
              const existing = document.getElementById(dev.uniquename);
              if (dev.onFloorplan === true) {
                try {
                  dev.htmlMinimum(existing);
                } catch (err) {
                  NotyHelper.generate_noty('error', '<b>Device draw error</b><br>' + err, false);
                }
              }
            }
          } else {
            dev = DomoticzDevicesService.Device.create(item);
            const existing = document.getElementById(dev.uniquename);
            if (dev.onFloorplan === true) {
              try {
                dev.htmlMinimum(existing);
              } catch (err) {
                NotyHelper.generate_noty('error', '<b>Device draw error</b><br>' + err, false);
              }
            }
          }
        });
        elIcons.setAttribute('id', this.floorPlans[floorIdx].tagName + '_Icons');
        elDetails.setAttribute('id', this.floorPlans[floorIdx].tagName + '_Details');
      }
    });
  }

  debugOn() {
    this.debug = 1;
  }

  debugOff() {
    this.debug = 0;
  }

}

interface FloorplanEnriched extends Floorplan {
  Loaded?: boolean;
  tagName?: string;
  yImageSize?: number;
  xImageSize?: number;
  floorID?: string;
}
