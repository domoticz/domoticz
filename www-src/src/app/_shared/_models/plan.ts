import {ApiResponse} from './api';

export interface Plans extends ApiResponse {
  result: Array<Plan>;
}

export class Plan {
  Devices: number;
  Name: string;
  Order: string;
  idx: string;
}

export interface UnusedPlanDevicesResponse extends ApiResponse {
  result: Array<UnusedPlanDevice>;
}

export interface UnusedPlanDevice {
  type: number;
  idx: string;
  Name: string;
}

export interface PlanDevicesResponse extends ApiResponse {
  result: Array<PlanDevice>;
}

export interface PlanDevice {
  DevSceneRowID: string;
  Name: string;
  devidx: string;
  idx: string;
  order: string;
  type: number;
}
