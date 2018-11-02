import {ApiResponse} from '../_shared/_models/api';

export interface LocationResponse extends ApiResponse {
  Latitude: string;
  Longitude: string;
}
