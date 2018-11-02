import {ApiResponse} from '../../../_shared/_models/api';

export interface InfluxConfigResponse extends ApiResponse {
  InfluxPassword: string;
  InfluxUsername: string;
  InfluxPath: string;
  InfluxPort: string;
  InfluxIP: string;
  InfluxDatabase: string;
  InfluxActive: number;
  InfluxDebug: number;
}

export interface InfluxLinksResponse extends ApiResponse {
  result: Array<InfluxLink>;
}

export interface InfluxLink {
  Delimitedvalue: string;
  DeviceID: string;
  Enabled: string;
  IncludeUnit: string;
  Name: string;
  TargetDevice: string;
  TargetProperty: string;
  TargetType: string;
  TargetVariable: string;
  idx: string;
}

