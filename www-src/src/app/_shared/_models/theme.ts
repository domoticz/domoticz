import { ApiResponse } from './api';

export interface ThemesResponse extends ApiResponse {
    result: Array<Theme>;
}

export interface Theme {
    theme: string;
}
