import {Injectable} from '@angular/core';
import {ApiService} from '../../_shared/_services/api.service';
import {Observable, of, throwError} from 'rxjs';
import {
  EventCurrentState,
  EventResponse,
  EventsCurrentStatesResponse,
  EventsResponse,
  EventTemplateResponse,
  EventToUpdate,
  FetchEvents,
  FullDzEvent
} from '../../_shared/_models/events';
import {map, mergeMap} from 'rxjs/operators';
import {ApiResponse} from '../../_shared/_models/api';
import {HttpParams} from '@angular/common/http';

@Injectable()
export class EventsService {

  constructor(private apiService: ApiService) {
  }

  fetchEvent(eventId: string): Observable<FullDzEvent> {
    return this.apiService.callApi<EventResponse>('events', {
      param: 'load',
      event: eventId
    }).pipe(
      map((data) => {
        return data.result[0];
      })
    );
  }

  fetchEvents(): Observable<FetchEvents> {
    return this.apiService.callApi<EventsResponse>('events', {param: 'list'}).pipe(
      map(data => {
        return {
          events: data.result || [],
          interpreters: data.interpreters ? data.interpreters.split(':') : []
        };
      })
    );
  }

  getTemplate(interpreter: string, eventType?: string): Observable<string> {
    return this.apiService.callApi<EventTemplateResponse>(
      'events', {
        param: 'new',
        interpreter: interpreter,
        eventtype: eventType || 'All'
      }).pipe(
      map(response => {
        return response.template;
      })
    );
  }

  fetchCurrentStates(): Observable<Array<EventCurrentState>> {
    return this.apiService.callApi<EventsCurrentStatesResponse>('events', {param: 'currentstates'})
      .pipe(
        mergeMap(response => {
          if (response && response.status !== 'OK') {
            return throwError(response);
          } else {
            return of(response);
          }
        }),
        map((response: EventsCurrentStatesResponse) => {
          return response.result || [];
        })
      );
  }

  deleteEvent(eventId: string): Observable<ApiResponse> {
    return this.apiService.callApi('events', {param: 'delete', event: eventId});
  }

  updateEvent(event: EventToUpdate): Observable<ApiResponse> {
    // 'Content-Type': 'application/x-www-form-urlencoded'
    const params = new HttpParams()
      .set('eventid', event.id || '')
      .set('name', event.name)
      .set('interpreter', event.interpreter)
      .set('eventtype', event.type)
      .set('xml', event.xmlstatement)
      .set('logicarray', event.logicarray)
      .set('eventstatus', event.eventstatus);
    return this.apiService.post<ApiResponse>('event_create.webem', params).pipe(
      mergeMap(response => {
        if (response && response.status !== 'OK') {
          return throwError(response);
        } else {
          return of(response);
        }
      })
    );
  }

  updateEventState(eventId: string, isEnabled: boolean): Observable<ApiResponse> {
    return this.apiService.callApi('events', {
      param: 'updatestatus',
      eventid: eventId,
      eventstatus: isEnabled ? '1' : '0'
    });
  }

}
