import { ApiResponse } from '../../_shared/_models/api';

export interface KodiNodesResponse extends ApiResponse {
    result: Array<KodiNode>;
}

export interface KodiNode {
    Port: any;
    IP: any;
    Name: any;
    idx: any;
}
