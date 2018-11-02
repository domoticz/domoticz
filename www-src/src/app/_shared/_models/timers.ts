import {ApiResponse} from './api';

export interface TimersResponse extends ApiResponse {
  result: Array<Timer>;
}

export class Timer {
  Active: string;
  Cmd?: number;
  Date: string;
  Days: number;
  MDay: number;
  Month: number;
  Occurence: number;
  Temperature: number;
  Time: string;
  Type: number;
  idx: string;
  Level?: number;
  Color?: string;
}

export interface TimerPlansResponse extends ApiResponse {
  result: Array<TimerPlan>;
}

export interface TimerPlan {
  idx: string;
  Name: string;
}
