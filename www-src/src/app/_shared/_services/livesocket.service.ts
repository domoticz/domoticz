import {Injectable} from '@angular/core';
import {Observable, Subject} from 'rxjs';
import {webSocket, WebSocketSubject, WebSocketSubjectConfig} from 'rxjs/webSocket';
import {HttpClient} from '@angular/common/http';
import {NotifyBrowserService} from './notify-browser.service';
import {TimeAndSun} from '../_models/sunriseset';

// tslint:disable:max-line-length
/*
	The livesocket service connects a websocket to domoticz in the Init() method.
	This socket connection stays live through all the interface page life.
	Via the websocket, notifications are pushed by Domoticz sending a msg.event == 'notification' object.
	Furthermore, get requests can be issued by the getJson(url, callback_fn) method. The url will be the same as if
	passed through an ajax call. The callback function can also be the same, ergo this function is designed to replace the usual ajax requests.
	An added feature is that devices that are retrieved via the json call, also from then on get real time status updates via a broadcast message.
	These status updates can be updated in a live manner. Example (taken from UtilityController.js):
		$scope.$on('jsonupdate', function (event, data) {
			RefreshItem(data.item);
		});
	With this, periodic ajax requests are not neccesary anymore. As the moment there is a device update, the new information gets broadcasted
	immediately.
*/
@Injectable()
export class LivesocketService {

  private websocket: WebSocketSubject<any>;
  private requestsCount = 0;
  private requestsQueue: Array<RequestInfo> = [];

  public jsonupdate = new Subject<any>();
  public scene_update = new Subject<any>();
  public time_update = new Subject<TimeAndSun>();
  public device_update = new Subject<any>();

  constructor(
    private httpClient: HttpClient,
    private notifyBrowserService: NotifyBrowserService) {
    this.init();
  }

  public init() {
    const wsProtocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    // FIXME: there could be an issue if Domoticz is setup on a subpath
    // Historic code was:
    // const wsURI = wsProtocol + '//' + location.host + location.pathname + 'json';
    const wsURI = wsProtocol + '//' + location.host + '/json';

    // TODO options ?
    // lazy: false,
    //   reconnect: true,
    //   reconnectInterval: 2000,
    //   enqueue: true

    const socketOptions: WebSocketSubjectConfig<any> = {
      url: wsURI,
      protocol: ['domoticz']
    };

    this.websocket = webSocket(socketOptions);

    this.websocket.subscribe((msg: any) => {
        this.handleMessage(msg);
      },
      (err) => {
        console.log('websocket error', err);
      },
      () => {
        console.log('websocket closed');
      }
    );
  }

  private handleMessage(msg: any) {

    if (typeof msg === 'string') {
      msg = JSON.parse(msg);
    }
    switch (msg.event) {
      case 'notification':
        this.notifyBrowserService.notify(msg.Subject, msg.Text);
        return;
      case 'date_time':
        this.handleTimeUpdate(msg);
        return;
    }
    if (msg.requestid >= 0) {
      this.handleRequestResponse(msg);
    } else {
      const data = msg.data ? JSON.parse(msg.data) : msg;

      if (msg.request === 'device_request' && data.status === 'OK') {
        data.result.forEach((device) => {
          this.device_update.next(device);
        });
        this.handleTimeUpdate(data);
      }

      if (msg.request === 'scene_request' && data.status === 'OK') {
        data.result.forEach((item) => {
          this.scene_update.next(item);
        });

        this.handleTimeUpdate(data);
      }
    }
  }

  private handleTimeUpdate(msg: TimeAndSun) {
    this.time_update.next({
      ServerTime: msg.ServerTime,
      Sunrise: msg.Sunrise,
      Sunset: msg.Sunset
    });
  }

  private handleRequestResponse(msg: any) {
    const requestIndex = this.requestsQueue.findIndex((item) => {
      return item.requestId === msg.requestid;
    });

    if (requestIndex === -1) {
      return;
    }

    const requestInfo = this.requestsQueue[requestIndex];
    const payload = msg.data ? JSON.parse(msg.data) : msg;
    requestInfo.callback.next(payload);
    this.requestsQueue.splice(requestIndex, 1);
  }

  /**
   * @deprecated prefer to use ApiService service instead
   */
  public getJson<T>(url: string, callback_fn?: (data?: T) => void): void {
    if (!callback_fn) {
      callback_fn = (data) => {
        this.jsonupdate.next(data);
      };
    }
    const use_http = !(url.substr(0, 9) === 'json.htm?');

    if (use_http) {
      this.httpClient.get(url).subscribe((data: any) => {
        callback_fn(data);
      });
    } else {
      this.sendRequest<T>(url.substr(9)).subscribe((data) => {
        callback_fn(data);
      });
    }
  }

  public sendRequest<T>(url: string): Observable<T> {

    const requestId = ++this.requestsCount;

    const requestobj = {
      event: 'request',
      requestid: requestId,
      query: url
    };

    const callback: Subject<T> = new Subject();
    const requestInfo = {
      requestId: requestId,
      callback: callback
    };

    this.requestsQueue.push(requestInfo);

    this.websocket.next(requestobj);

    return callback;
  }

}

interface RequestInfo {
  requestId: number;
  callback: Subject<any>;
}
