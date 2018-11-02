import { ApiResponse } from './api';

export interface LightSwitchesResponse extends ApiResponse {
    result: Array<LightSwitch>;
}

export interface LightSwitch {
    DimmerLevels: string;
    IsDimmer: boolean;
    Name: string;
    SubType: string;
    Type?: string;
    idx: string;
}

export interface SubDevicesResponse extends ApiResponse {
    result: Array<any>;
}

export interface LightSwitchesScenesResponse extends ApiResponse {
    result: Array<LightSwitchScene>;
}

export interface LightSwitchScene {
    Name: string;
    idx: string;
    type: number;
}
