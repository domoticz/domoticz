import { ApiResponse } from './api';

export interface CamerasResponse extends ApiResponse {
    result: Array<Camera>;
}

export interface Camera {
    Address: string;
    Enabled: string;
    ImageURL: string;
    Name: string;
    Password: string;
    Port: number;
    Protocol: number;
    Username: string;
    idx: string;
}

export interface CameraDevicesResponse extends ApiResponse {
    result: Array<CameraDevice>;
}

export interface CameraDevice {
    DevSceneRowID: string;
    Name: string;
    delay: number;
    idx: string;
    type: number;
    when: number;
}
