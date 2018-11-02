import {ApiResponse} from './api';

export interface UserVariablesResponse extends ApiResponse {
  result: Array<UserVariable>;
}

export interface UserVariable {
  'LastUpdate': string;
  'Name': string;
  'Type': string;
  'Value': string;
  'idx': string;
}
