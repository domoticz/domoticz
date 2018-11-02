import {ConfigService} from '../_shared/_services/config.service';
import {Component, OnInit} from '@angular/core';
import {HistoryService} from './history.service';
import {History} from './history';

@Component({
  selector: 'dz-history',
  templateUrl: './history.component.html',
  styleUrls: ['./history.component.css']
})
export class HistoryComponent implements OnInit {

  history: History;

  constructor(private configService: ConfigService,
              private historyService: HistoryService) {
  }

  ngOnInit() {
    let bShowNewHistory = false;

    if (this.configService.globals.historytype && this.configService.globals.historytype === 2) {
      bShowNewHistory = true;
    }

    if (bShowNewHistory === false) {
      this.historyService.getActualHistory().subscribe(history => {
        this.history = history;
      });
    } else {
      this.historyService.getNewHistory().subscribe(history => {
        this.history = history;
      });
    }
  }

}
