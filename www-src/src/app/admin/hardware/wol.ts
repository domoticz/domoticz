import { ApiResponse } from '../../_shared/_models/api';

export interface WolNodesResponse extends ApiResponse {
    result: Array<WolNode>;
}

export interface WolNode {
    idx: string;
    Name: string;
    Mac: string;
}
