import {ApiResponse} from './api';
import {SunRiseSet} from "./sunriseset";

export interface ListDevicesResponse extends ApiResponse {
    result: Array<{ name: string; value: string }>;
}

export interface Wording extends ApiResponse {
  wording: string;
}

export interface DeviceValueOptionsResponse extends ApiResponse {
  result: Array<DeviceValueOption>;
}

export interface DeviceValueOption {
  Value: number;
  Wording: string;
}

export interface DevicesResponse extends ApiResponse, SunRiseSet {
  AllowWidgetOrdering?: boolean;
  ActTime?: number;
  app_version: string;
  result: Array<Device>;
}

export interface Device {
  AddjMulti: number;
  AddjMulti2: number;
  AddjValue: number;
  AddjValue2: number;
  Altitude?: any; // FIXME type
  Barometer?: number;
  BatteryLevel: number;
  CameraIdx?: string;
  Chill?: number;
  Color?: string;
  Counter?: string;
  CounterDeliv?: string;
  CounterDelivToday?: string;
  CounterToday?: string;
  CustomImage: number;
  Data: string;
  DayTime?: string;
  Desc?: string;
  Description: string;
  DewPoint?: string;
  DimmerType?: any; // FIXME type
  Direction?: number;
  DirectionStr?: string;
  displaytype?: number;
  EnergyMeterMode?: string;
  Favorite: number;
  Gust?: string;
  Forecast?: number;
  ForecastStr?: string;
  forecast_url?: string;
  HardwareID: number;
  HardwareName: string;
  HardwareType: string;
  HardwareTypeVal: number;
  HaveDimmer?: boolean;
  HaveGroupCmd?: boolean;
  HaveTimeout: boolean;
  Humidity?: number;
  HumidityStatus?: string;
  ID: string;
  InternalState?: string;
  Image?: string;
  IsSubDevice?: boolean;
  LastUpdate: string;
  Level?: any; // FIXME type
  LevelActions?: string;
  LevelInt?: number;
  LevelNames?: string;
  LevelOffHidden?: boolean;
  MaxDimLevel?: any;
  Mode?: any; // FIXME type
  Modes?: any; // FIXME type
  Name: string;
  Notifications: string;
  OffAction?: string;
  OnAction?: string;
  PlanID: string;
  PlanIDs: Array<number>;
  Protected: boolean;
  Quality?: string;
  Rain?: string;
  RainRate?: string;
  Radiation?: number;
  SelectorStyle?: string;
  SensorType?: string;
  SensorUnit?: string;
  SetPoint?: number;
  ShowNotifications: boolean;
  SignalLevel: any;
  Speed?: string;
  State?: string;
  Status?: string;
  StrParam1?: string;
  StrParam2?: string;
  SubType: string;
  SwitchType?: string;
  SwitchTypeVal?: number;
  Temp?: number;
  Timers: string;
  Type: string;
  TypeImg: string;
  trend?: number;
  Unit: number | string;
  Until?: string;
  Usage?: string;
  UsageDeliv?: string;
  Used: number;
  UsedByCamera?: boolean;
  UVI?: string;
  ValueQuantity?: string;
  ValueUnits?: string;
  Visibility?: number;
  XOffset: string;
  YOffset: string;
  idx: string;
}
