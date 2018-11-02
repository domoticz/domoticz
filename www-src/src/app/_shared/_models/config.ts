export class GlobalConfig {
  EnableTabProxy: boolean;
  EnableTabDashboard: boolean;
  EnableTabFloorplans: boolean;
  EnableTabLights: boolean;
  EnableTabScenes: boolean;
  EnableTabTemp: boolean;
  EnableTabWeather: boolean;
  EnableTabUtility: boolean;
  EnableTabCustom: boolean;
  AllowWidgetOrdering: boolean;
  FiveMinuteHistoryDays: number;
  DashboardType: number;
  MobileType: number;
  TempScale: number;
  DegreeDaysBaseTemperature: number;
  TempSign: string;
  WindScale: number;
  WindSign: string;
  language: string;
  HaveUpdate: boolean;
  UseUpdate: boolean;
  // appversion: string;
  apphash: string;
  appdate: string;
  pythonversion: string;
  versiontooltip: string;
  ShowUpdatedEffect: boolean;
  DateFormat: string;
  dzventsversion?: string;
  isproxied: boolean;
  Latitude?: number;
}

// It's the equivalent of previous $.myglobals.xxx
export class GlobalVars {
  ismobileint: boolean;
  ismobile: boolean;
  actlayout: string;
  prevlayout: string;
  historytype: number;
  LastPlanSelected: number;
  DashboardType: number;
  isproxied: boolean;
  AnimateTransitions?: any;
  FullscreenMode?: boolean;
  RoomColour?: string;
  ActiveRoomOpacity?: number;
  InactiveRoomOpacity?: number;
  HardwareTypesStr: Array<string>;
  HardwareI2CStr: Array<string>;
  SelectedHardwareIdx: number;
  LastUpdate: number;
}
