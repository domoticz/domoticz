import {ApiResponse} from './api';

export interface CheckForUpdateResponse extends ApiResponse {
  DomoticzUpdateURL: string;
  HaveUpdate: boolean;
  Revision: number;
  SystemName: string;
}

export interface DownloadReadyResponse extends ApiResponse {
  downloadok: boolean;
}
