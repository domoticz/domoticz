import { Component, OnInit, Input, EventEmitter, Output, AfterViewInit, OnChanges, SimpleChanges, ViewChild, ElementRef } from '@angular/core';
import { RgbwPickerService } from './rgbw-picker.service';

@Component({
  selector: 'dz-rgbw-picker',
  templateUrl: './rgbw-picker.component.html',
  styleUrls: ['./rgbw-picker.component.css']
})
export class RgbwPickerComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() color: string;
  @Output() colorChange: EventEmitter<string> = new EventEmitter<string>();

  @Input() level: number;
  @Output() levelChange: EventEmitter<number> = new EventEmitter<number>();

  @Input() dimmerType: any;
  @Input() maxDimmerLevel: number;
  @Input() colorSettingsType: string;

  @Input() deviceIdx?: string = null;

  @ViewChild('picker', { static: true }) pickerRef: ElementRef;

  currentValue: any;

  constructor(
    private rgbwPickerService: RgbwPickerService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.render(this.getColor());
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.color || changes.level || changes.deviceIdx) {
      this.render(this.getColor());
    }
  }

  render(value: any) {
    if (value === this.currentValue) {
      return;
    }

    this.rgbwPickerService.ShowRGBWPicker(
      this.pickerRef,
      this.deviceIdx,
      0,
      this.maxDimmerLevel || 100,
      this.level,
      this.color,
      this.colorSettingsType,
      this.dimmerType,
      (devIdx, JSONColor, dimlevel) => this.onColorChange(devIdx, JSONColor, dimlevel)
    );

    this.currentValue = value;
  }

  onColorChange(idx, color, level) {
    this.color = color;
    this.level = level;
    this.currentValue = this.getColor();

    // $scope.$apply();
    this.colorChange.emit(this.color);
    this.levelChange.emit(this.level);
  }

  public getColor() {
    return this.color + this.level;
  }

}
