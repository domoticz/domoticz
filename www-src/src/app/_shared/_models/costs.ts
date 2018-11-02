import { ApiResponse } from './api';

export interface Costs {
    CostEnergy: number; // also called CostsT1
    CostEnergyR1: number;
    CostEnergyR2: number;
    CostEnergyT2: number;
    CostGas: number;
    CostWater: number;
    CounterT1?: any; // FIXME type?
    CounterT2?: any; // FIXME type?
    CounterR1?: any; // FIXME type?
    CounterR2?: any; // FIXME type?
    DividerWater?: number;
}

export interface CostsResponse extends ApiResponse, Costs {
}
