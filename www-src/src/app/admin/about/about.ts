import { ApiResponse } from '../../_shared/_models/api';

export interface AboutResponse extends ApiResponse {
    days: number;
    hours: number;
    minutes: number;
    seconds: number;
}
