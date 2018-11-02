import { ApiResponse } from '../../_shared/_models/api';

export interface BleBoxNodesResponse extends ApiResponse {
    result: Array<BleBoxNode>;
}

export interface BleBoxNode {
  fv: any;
  hv: any;
  Uptime: any;
  Type: any;
  IP: any;
  Name: string;
  idx: any;
}
