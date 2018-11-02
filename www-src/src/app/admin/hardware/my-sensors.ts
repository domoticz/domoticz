import { ApiResponse } from '../../_shared/_models/api';

export interface MySensorsResponse extends ApiResponse {
    result: Array<MySensor>;
}

export interface MySensor {
    idx: string;
    Name: string;
    SketchName: string;
    Version: string;
    Childs: any;
    LastReceived: string;
}

export interface MySensorsChildrenResponse extends ApiResponse {
    result: Array<MySensorChild>;
}

export interface MySensorChild {
    child_id: any;
    use_ack: any;
    ack_timeout: any;
    type: any;
    name: any;
    Values: any;
    LastReceived: any;
}
