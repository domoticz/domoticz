import {ApiResponse} from './api';

export interface FloorplansResponse extends ApiResponse {
  ActiveRoomOpacity: number;
  AnimateZoom: number;
  FullscreenMode: number;
  InactiveRoomOpacity: number;
  PopupDelay: number;
  RoomColour: string;
  ShowSceneNames: number;
  ShowSensorValues: number;
  ShowSwitchValues: number;
  result: Array<Floorplan>;
}

export interface Floorplan {
  Image: string;
  Name: string;
  Order: string;
  Plans: number;
  ScaleFactor: string;
  idx: string;
}

export interface FloorplanResponse extends ApiResponse {
  result: Array<FloorplanItem>;
}

export interface FloorplanItem {
  Area: string;
  Name: string;
  idx: string;
}

export interface UnusedFloorplanPlansResponse extends ApiResponse {
  result: Array<UnusedFloorplanPlan>;
}

export interface UnusedFloorplanPlan {
  idx: string;
  Name: string;
}

