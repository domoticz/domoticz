import {ApiResponse} from '../../_shared/_models/api';

export interface SecurityResponse extends ApiResponse {
  secondelay: number;
  secstatus: number;
}
