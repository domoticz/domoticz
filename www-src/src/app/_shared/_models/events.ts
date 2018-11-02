import {ApiResponse} from './api';

export interface EventsResponse extends ApiResponse {
  interpreters: string;
  result: Array<DzEvent>;
}

export interface DzEvent {
  eventstatus: string;
  id: string;
  name: string;
}

export interface FullDzEvent extends DzEvent {
  interpreter: string;
  type: string;
  xmlstatement: string;
}

export interface EventTemplateResponse extends ApiResponse {
  template: string;
}

export interface EventsCurrentStatesResponse extends ApiResponse {
  result: Array<EventCurrentState>;
}

export interface EventCurrentState {
  id: number;
  lastupdate: string;
  name: string;
  value: string;
  values: string;
}

export interface EventResponse extends ApiResponse {
  editortheme: string;
  result: Array<FullDzEvent>;
}

export interface EventToUpdate extends FullDzEvent {
  logicarray?: string;
}

export interface FetchEvents {
  events: Array<DzEvent>;
  interpreters: Array<string>;
}
