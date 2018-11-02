import { ApiResponse } from './api';

export interface LogRespone extends ApiResponse {
    LastLogTime: string;
    result: Array<LogMessage>;
}

export interface LogMessage {
    level: number;
    message: string;
}
