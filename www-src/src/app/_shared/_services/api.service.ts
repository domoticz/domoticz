import {Injectable} from '@angular/core';
import {HttpClient, HttpHeaders} from '@angular/common/http';
import {Observable} from 'rxjs';

@Injectable()
export class ApiService {

  constructor(private httpClient: HttpClient) {
  }

  callApi<T>(type: string, params: { [param: string]: string | string[]; }): Observable<T> {
    const url = 'json.htm';
    const fullParams = {...params, type: type};
    return this.httpClient.get<T>(url, {
      params: fullParams
    });
  }

  post<T>(uri: string, body: any, headers?: HttpHeaders | { [p: string]: string | string[] }): Observable<T> {
    return this.httpClient.post<T>(uri, body, {headers: headers});
  }

  postFormData<T>(uri: string, body: FormData): Observable<T> {
    return this.httpClient.post<T>(uri, body, {
      headers: {
        'Content-Type': 'multipart/form-data'
      }
    });
  }

}
