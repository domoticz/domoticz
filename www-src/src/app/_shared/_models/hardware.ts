import { ApiResponse } from './api';

export interface HardwareResponse extends ApiResponse {
    result: Array<Hardware>;
}

export interface Hardware {
    Address: string;
    DataTimeout: number;
    Enabled: string;
    Extra: string;
    Mode1: number;
    Mode2: number;
    Mode3: number;
    Mode4: number;
    Mode5: number;
    Mode6: number;
    Name: string;
    NodesQueried?: boolean;
    Password: string;
    Port: number;
    SerialPort: string;
    Type: number;
    Username: string;
    idx: number | string; // Depends on which endpoint is used... WTF!
    noiselvl: number;
    version: string;
}

export interface HardwareTypesResponse extends ApiResponse {
    result: Array<HardwareType>;
}

export interface HardwareType {
    idx: number;
    name: string;
    author?: string;
    description?: string;
    externalURL?: string;
    key?: string;
    parameters?: Array<HardwareTypeParameter>;
    wikiURL?: string;
}

export interface HardwareTypeParameter {
    default: string;
    field: string;
    label: string;
    password: string;
    required: string;
    width: string;
    options?: Array<HardwareTypeParameterOption>;
}

export interface HardwareTypeParameterOption {
    label: string;
    value: string;
    default?: string;
}

export interface SerialDevicesResponse extends ApiResponse {
    result: Array<{ name: string, value: number }>;
}

export interface RegisterHueResponse extends ApiResponse {
    username: string;
}

export interface FirmwareUpdatePercentageResponse extends ApiResponse {
    percentage: number;
}

export interface LmsNodesResponse extends ApiResponse {
    result: Array<any>;
}

export interface EvoHomeResponse extends ApiResponse {
    Used: boolean;
    Name: string;
}

export interface ZWaveNodesResponse extends ApiResponse {
    NodesQueried: boolean;
    ownNodeId: number;
    result: Array<ZWaveNode>;
}

export interface ZWaveNode {
    Description: string;
    Generic_type: string;
    HaveUserCodes: boolean;
    HomeID: number;
    IsPlus: boolean;
    LastUpdate: string;
    Manufacturer_id: string;
    Manufacturer_name: string;
    Name: string;
    NodeID: number;
    PollEnabled: string;
    Product_id: string;
    Product_name: string;
    Product_type: string;
    State: string;
    Version: number;
    config: Array<ZWaveNodeConfig>;
    idx: string;
    Battery: string;
}

export interface ZWaveNodeConfig {
    LastUpdate: string;
    help: string;
    index: number;
    label: string;
    list_items?: number;
    listitem?: string[];
    type: string;
    units: string;
    value: string | number;
}
