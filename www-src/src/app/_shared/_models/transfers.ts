import { ApiResponse } from './api';

export interface Transfers extends ApiResponse {
    result?: Array<TransferableItem>;
}

export interface TransferableItem {
    Name: string;
    idx: string;
}
