import { ApiResponse } from './api';

export interface UsersResponse extends ApiResponse {
    result: Array<User>;
}

export interface User {
    Enabled: string;
    Password: string;
    RemoteSharing: number;
    Rights: number;
    TabsEnabled: number;
    Username: string;
    idx: string;
}

export interface UserDevicesResponse extends ApiResponse {
    result: Array<UserDevice>;
}

export interface UserDevice {
    DeviceRowIdx: string;
}
