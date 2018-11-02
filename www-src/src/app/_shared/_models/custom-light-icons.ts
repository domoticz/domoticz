export interface CustomLightIconsResponse {
    result: Array<CustomLightIconApi>;
    status: string;
}

export interface CustomLightIconApi {
    description: string;
    idx: number;
    imageSrc: string;
    text: string;
}

export interface CustomLightIcon {
    text: string;
    value: number;
    selected: boolean;
    description: string;
    imageSrc?: string;
}
