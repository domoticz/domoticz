import {ApiResponse} from '../../../_shared/_models/api';

export interface GooglePubSubConfigResponse extends ApiResponse {
  GooglePubSubData: string;
  GooglePubSubActive: number;
  GooglePubSubDebug: number;
}

export interface GooglePubSubLinksResponse extends ApiResponse {
  result: Array<GooglePubSubLink>;
}

export interface GooglePubSubLink {
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

