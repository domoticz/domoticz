import { ApiResponse } from './api';

export interface GraphData extends ApiResponse {
    haveL3: any;
    haveL2: any;
    result: Array<GraphTempPoint>;
    resultprev?: Array<GraphTempPoint>;
    result_speed?: Array<any>;
    ValueQuantity?: string;
    ValueUnits?: string;
    delivered?: any; // FIXME type?
    haveL1: boolean;
    counter: any; // FIXME type?
}

export interface GraphTempPoint {
    d: string;
    ta?: number;
    te?: number;
    tm?: number;
    hu?: number; // Warning this one is actually a string in WS response but transformed to number manually
    ch?: number;
    cm?: number;
    dp?: number;
    ba?: number;
    se?: number;
    sm?: number;
    sx?: number;
    mm?: number;
    uvi?: number;
    gu?: number;
    sp?: number;
    v?: number;
    v1?: number;
    v2?: number;
    v3?: number;
    v4?: number;
    v5?: number;
    v6?: number;
    v_min?: number;
    v_max?: number;
    r1?: number;
    r2?: number;
    eu?: number;
    eg?: number;
    co2?: number;
    co2_avg?: number;
    co2_min?: number;
    co2_max?: number;
    c?: number;
    c1?: number;
    c2?: number;
    c3?: number;
    c4?: number;
}
