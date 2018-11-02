import { ApiResponse } from '../../_shared/_models/api';

export interface ZWaveIncludedNodeResponse extends ApiResponse {
    result: boolean;
    node_id: string;
    node_product_name: string;
}

export interface ZWaveExcludedNodeResponse extends ApiResponse {
    result: boolean;
    node_id: string;
}

export interface ZWaveUserCodesResponse extends ApiResponse {
    result: Array<ZWaveUserCode>;
}

export interface ZWaveUserCode {
    index: any;
    code: string;
}

export interface ZWaveNetworkInfoResponse extends ApiResponse {
    result: {
        mesh: Array<ZWaveMeshNode>;
        nodes: string;
    };
}

export interface ZWaveMeshNode {
    nodeID: number;
    seesNodes: string;
}

export interface ZWaveGroupInfoResponse extends ApiResponse {
    result: {
        MaxNoOfGroups: number;
        nodes: Array<ZWaveGroupNode>;
    };
}

export interface ZWaveGroupNode {
    groupCount: number;
    nodeID: number;
    nodeName: string;
    groups?: Array<ZWaveGroup>;
}

export interface ZWaveGroup {
    groupName: string;
    id: number;
    nodes: string;
}
