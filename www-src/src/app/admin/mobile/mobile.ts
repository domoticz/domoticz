import { ApiResponse } from '../../_shared/_models/api';

export interface MobilesResponse extends ApiResponse {
    result: Array<Mobile>;
}

export interface Mobile {
    Enabled: string;
    LastUpdate: string;
    idx: string;
    Name: string;
    UUID: string;
    DeviceType: string;
}
