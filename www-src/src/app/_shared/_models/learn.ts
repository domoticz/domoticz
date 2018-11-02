import { ApiResponse } from './api';

export interface Learnsw extends ApiResponse {
    ID: string;
    idx: number;
    Used: boolean;
    Name: string;
    Cmd?: number;
}
