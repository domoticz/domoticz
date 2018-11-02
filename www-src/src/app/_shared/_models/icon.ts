import { ApiResponse } from './api';

export interface IconsetResponse extends ApiResponse {
    result: Array<Icon>;
}

export interface Icon {
    selected: boolean;
    Title: string;
    Description: string;
    IconFile16: string;
    IconFile48On: string;
    IconFile48Off: string;
    idx: string;
}
