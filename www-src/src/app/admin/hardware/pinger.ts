import { ApiResponse } from '../../_shared/_models/api';

export interface PingerNodesResponse extends ApiResponse {
    result: Array<PingerNode>;
}

export interface PingerNode {
    idx: any;
    Name: any;
    IP: any;
    Timeout: any;
}
