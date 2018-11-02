import {ApiResponse} from '../../../_shared/_models/api';

export interface FibaroConfig extends ApiResponse {
  FibaroIP: string;
  FibaroUsername: string;
  FibaroPassword: string;
  FibaroActive: number;
  FibaroDebug: number;
  FibaroVersion4: number;
}

export interface FibaroLinksResponse extends ApiResponse {
  result: Array<FibaroLink>;
}

export interface FibaroLink {
  idx: string;
  TargetProperty: string;
  TargetVariable: string;
  Name: string;
  DeviceID: string;
  Delimitedvalue: string;
  TargetDevice: string;
  TargetType: string;
  IncludeUnit: string;
  Enabled: string;
}

