import { Component, OnInit } from '@angular/core';
import { NotifyBrowserService, MessageItem } from '../../_services/notify-browser.service';

/*
	The alertarea is on every page. It displays the browser notifications.
	It uses the notifyBrowser service to collect the pending notifications.
	Todo: Display them below each other instead of left to each other.
	Todo: Make a "dropdown" css effect
*/
@Component({
  selector: 'dz-alertarea',
  templateUrl: './alertarea.component.html',
  styleUrls: ['./alertarea.component.css']
})
export class AlertareaComponent implements OnInit {

  constructor(private notifyBrowserService: NotifyBrowserService) { }

  ngOnInit() {
  }

  get messages(): Array<MessageItem> {
    return this.notifyBrowserService.messages();
  }

}
