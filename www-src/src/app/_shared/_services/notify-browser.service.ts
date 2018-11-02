import {Injectable} from '@angular/core';
import {timer} from 'rxjs';

/*
	The notifyBrowser service sets and gets the browser notifications coming on from the domoticz websocket.
	In livesocket.js the notifications are pushed to the message queue by using the notify() method.
	Each notification times out in 2 seconds.
	The alertarea directive reads the messagequeue using the messages() method.
*/
@Injectable()
export class NotifyBrowserService {

  private messagequeue: Array<MessageItem> = [];

  constructor() {
    // request permission on page load
    if (Notification.permission !== 'granted') {
      Notification.requestPermission();
    }
  }

  public notify(title: string, body: string): void {
    const item = {'title': title, 'body': body, 'time': new Date()};
    this.messagequeue.push(item);

    timer(2000).subscribe(() => {
      const index = this.messagequeue.indexOf(item);
      if (index >= 0) {
        this.messagequeue.splice(index, 1);
      }
    });

    if (typeof Notification === 'undefined') {
      console.log('Notification: ' + title + ': ' + body);
      console.log('Desktop notifications not available in your browser. Try Chromium.');
      return;
    }

    if (Notification.permission !== 'granted') {
      Notification.requestPermission();
    } else {
      const notification = new Notification(title, {
        // icon: 'http://cdn.sstatic.net/stackexchange/img/logos/so/so-icon.png',
        body: body,
      });

      notification.onclick = () => {
        window.open('http://stackoverflow.com/a/13328397/1269037'); // FIXME really ?!
      };
    }
  }

  public messages(): Array<MessageItem> {
    return this.messagequeue;
  }

}

export interface MessageItem {
  title: string;
  body: string;
  time: Date;
}
