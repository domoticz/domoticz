import { ApiResponse } from '../../_shared/_models/api';

export interface PanasonicTvNodesResponse extends ApiResponse {
    result: Array<PanasonicTvNode>;
}

export interface PanasonicTvNode {
    Port: any;
    IP: any;
    Name: string;
    idx: any;
}
