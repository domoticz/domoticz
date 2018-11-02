import { Component, OnInit, ElementRef, ViewChild, AfterViewInit, Input, Inject, OnDestroy, EventEmitter, Output, HostListener, ChangeDetectorRef } from '@angular/core';
import { Subject } from 'rxjs';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { DomSanitizer, SafeStyle } from '@angular/platform-browser';
import { LightSwitchesService } from '../../_services/light-switches.service';
import { mergeMap, debounceTime } from 'rxjs/operators';
import {Device} from "../../_models/device";

// FIXME proper declaration
declare var $: any;

// FIXME this could be a directive maybe
@Component({
  selector: 'dz-dimmer-slider',
  templateUrl: './dimmer-slider.component.html',
  styleUrls: ['./dimmer-slider.component.css']
})
export class DimmerSliderComponent implements OnInit, AfterViewInit {

  @Input() type: string;
  @Input() disabled?= false;
  @Input() isled?= false;
  @Input() _style: string;
  @Input() _class: string;
  @Input() mobileview?= false;

  @Input() item: Device;
  @Output() itemChange: EventEmitter<Device> = new EventEmitter<Device>();

  @ViewChild('dimslider', { static: false }) dimsliderRef: ElementRef;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private sanitizer: DomSanitizer,
    private lightSwitchesService: LightSwitchesService,
    private changeDetector: ChangeDetectorRef
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const _this = this;

    const dimValue: Subject<number> = new Subject<number>();
    if (this.type !== 'relay') {
      // Wait for 500ms before sending any request, and send only the last dimValue set
      dimValue.pipe(
        debounceTime(500),
        mergeMap(val => this.lightSwitchesService.SetDimValue(this.item.idx, val))
      ).subscribe(() => {
        // Do nothing
      });
    }

    $(this.dimsliderRef.nativeElement).slider({
      // Config
      range: 'min',
      min: 0,
      max: 15,
      value: 4,

      // Slider Events
      create: function (event, ui) {
        $(this).slider('option', 'max', _this.item.MaxDimLevel);
        // $(this).slider('option', 'type', _this.type);
        // $(this).slider('option', 'isprotected', _this.item.Protected);
        $(this).slider('value', _this.item.LevelInt);
        if (_this.disabled) {
          $(this).slider('option', 'disabled', true);
        }
      },
      slide: function (event, ui) { // When the slider is sliding
        const maxValue = _this.item.MaxDimLevel;
        const fPercentage = Number((100.0 / maxValue) * ui.value);

        if (fPercentage === 0) {
          if (_this.type === 'blinds') {
            _this.item.Status = 'Open';
          } else if (_this.type === 'blinds_inv') {
            _this.item.Status = 'Closed';
          } else {
            _this.item.Status = 'Off';
          }
        } else {
          _this.item.Status = 'Set Level: ' + fPercentage + ' %';
        }
        _this.itemChange.emit(_this.item);

        if (this.type !== 'relay') {
          dimValue.next(ui.value);
        }
      },
      stop: function (event, ui) {
        const idx = _this.item.idx;
        if (_this.type === 'relay') {
          _this.lightSwitchesService.SetDimValue(idx, ui.value).subscribe(() => {
            // Do nothing
          });
        }
      }
    });

  }

  @HostListener('window:resize', ['$event'])
  onResize(event) {
    this.ResizeDimSliders();
  }

  get style(): SafeStyle {
    return this.sanitizer.bypassSecurityTrustStyle(this._style);
  }

  // TODO maybe this can be done with CSS calc() ?
  public ResizeDimSliders() {
    if ($('#lightcontent #name').length > 0) { // Lights dashboard
      const i = this._style.indexOf('width:');
      if (i >= 0) {
        this._style = this._style.substring(0, i);
      }

      const width = Math.floor($('#lightcontent #name').width()) - 50;
      if (this._class.includes('dimsmall')) {
        this._style += `width:${(width - 48)}px;`;
      } else {
        this._style += `width:${width}px;`;
      }
    } else if ($('#dashcontent #name').length > 0) { // Favorites dashboard
      const i = this._style.indexOf('width:');
      if (i >= 0) {
        this._style = this._style.substring(0, i);
      }

      if (this._class.includes('dimslidersmalldouble')) {
        const width = Math.floor($('#dashcontent #name').width()) - 85;
        this._style += `width:${width}px;`;
      } else if (this._class.includes('dimslidernorm') || this._class.includes('dimslidersmall')) {
        if (this.mobileview === true) {
          const width = Math.floor($('.mobileitem').width()) - 63;
          this._style += `width:${width}px;`;
        } else {
          const width = Math.floor($('#dashcontent #name').width()) - 40;
          this._style += `width:${width}px;`;
        }
      }
    }
    // Force detect changes, otherwise no update on drag/drop
    this.changeDetector.detectChanges();
  }

}
