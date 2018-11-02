import {ApiResponse} from '../../../_shared/_models/api';

export interface HttpConfigResponse extends ApiResponse {
  HttpHeaders: string;
  HttpAuthBasicPassword: string;
  HttpAuthBasicLogin: string;
  HttpData: string;
  HttpUrl: string;
  HttpActive: number;
  HttpAuth: number;
  HttpDebug: number;
  HttpMethod: number;
}

export interface HttpLinksResponse extends ApiResponse {
  result: Array<HttpLink>;
}

export interface HttpLink {
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
