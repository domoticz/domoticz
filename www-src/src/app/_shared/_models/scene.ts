import { ApiResponse } from './api';

export interface SceneDevices extends ApiResponse {
    result: Array<SceneDevice>;
}

export interface SceneDevice {
    Color: string;
    Command: string;
    DevID: string;
    DevRealIdx: string;
    ID: string;
    Level: number;
    Name: string;
    OffDelay: number;
    OnDelay: number;
    Order: number;
    SubType: string;
    Type: string;
}

export interface SceneActivations extends ApiResponse {
    result: Array<SceneActivation>;
}

export interface SceneActivation {
    idx: string;
    code: any;
    name: string;
    codestr: string;
}

export interface SceneLogs extends ApiResponse {
    result: Array<SceneLog>;
}

export interface SceneLog {
    Data: string;
    Date: string;
    idx: string;
}
