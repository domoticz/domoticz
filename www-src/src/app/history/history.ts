export interface History {
    LastLogTime: string;
    result: Array<HistoryItem>;
    status: string;
    title: string;
}

export interface HistoryItem {
    level: number;
    message: string;
}
