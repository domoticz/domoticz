import { LogMessage } from '../../_shared/_models/log';
import { Component, OnInit, HostListener, AfterViewInit, OnDestroy } from '@angular/core';
import { Subscription, interval } from 'rxjs';
import { LogService } from 'src/app/admin/log/log.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-log',
  templateUrl: './log.component.html',
  styleUrls: ['./log.component.css']
})
export class LogComponent implements OnInit, AfterViewInit, OnDestroy {

  LastLogTime = 0;

  logitems: Array<LogItem> = [];
  logitems_status: Array<LogItem> = [];
  logitems_error: Array<LogItem> = [];
  logitems_debug: Array<LogItem> = [];

  filterExpr = '';

  private timer: Subscription;

  constructor(
    private logService: LogService
  ) { }

  ngOnInit() {
    this.LastLogTime = 0;
  }

  ngAfterViewInit() {
    this.RefreshLog();
    this.ResizeLogWindow();
  }

  ngOnDestroy() {
    if (this.timer) {
      this.timer.unsubscribe();
      this.timer = undefined;
    }
  }

  @HostListener('window:resize', ['$event'])
  onResize(event) {
    this.ResizeLogWindow();
  }

  private ResizeLogWindow() {
    const pheight = $(window).innerHeight();
    $('#logcontent #logdata').height(pheight - 160);
    $('#logcontent #logdata_status').height(pheight - 160);
    $('#logcontent #logdata_error').height(pheight - 160);
    $('#logcontent #logdata_debug').height(pheight - 160);
  }

  private RefreshLog() {
    if (this.timer) {
      this.timer.unsubscribe();
      this.timer = undefined;
    }

    let bWasAtBottom = true;
    const div = $('#logcontent #logdata');
    if (div != null) {
      if (div[0] != null) {
        const diff = Math.abs((div[0].scrollHeight - div.scrollTop()) - div.height());
        if (diff > 9) {
          bWasAtBottom = false;
        }
      }
    }

    const lastscrolltop = $('#logcontent #logdata').scrollTop();
    const llogtime = this.LastLogTime;

    this.logService.getLog(this.LastLogTime, LogService.LOG_ALL).subscribe(data => {
      if (typeof data.result !== 'undefined') {
        if (typeof data.LastLogTime !== 'undefined') {
          this.LastLogTime = Number(data.LastLogTime);
        }
        data.result.forEach((item: LogMessage, i: number) => {
          const message = item.message.replace(/\n/gi, '<br>');
          const lines = message.split('<br>');
          if (lines.length < 1) {
            return;
          }
          const fline = lines[0].split(' ');
          if (fline.length < 3) {
            return;
          }

          const sdate = fline[0];
          const stime = fline[1];

          for (i = 0; i < lines.length; i++) {
            let lmessage = '';
            if (i === 0) {
              lmessage = lines[i];
            } else {
              lmessage = sdate + ' ' + stime + ' ' + lines[i];
            }
            let logclass = '';
            logclass = this.getLogClass(item.level);
            this.logitems = this.logitems.concat({
              mclass: logclass,
              text: lmessage
            });
            if (this.logitems.length >= 300) {
              this.logitems.splice(0, (this.logitems.length - 300));
            }
            if (item.level === LogService.LOG_ERROR) {
              // Error
              this.logitems_error = this.logitems_error.concat({
                mclass: logclass,
                text: lmessage
              });
              if (this.logitems_error.length >= 300) {
                this.logitems_error.splice(0, (this.logitems_error.length - 300));
              }
            } else if (item.level === LogService.LOG_STATUS) {
              // Status
              this.logitems_status = this.logitems_status.concat({
                mclass: logclass,
                text: lmessage
              });
              if (this.logitems_status.length >= 300) {
                this.logitems_status.splice(0, (this.logitems_status.length - 300));
              }
            } else if (item.level === LogService.LOG_DEBUG) {
              // Debug
              this.logitems_debug = this.logitems_debug.concat({
                mclass: logclass,
                text: lmessage
              });
              if (this.logitems_debug.length >= 300) {
                this.logitems_debug.splice(0, (this.logitems_debug.length - 300));
              }
            }
          }
        });
      }

      this.timer = interval(5000).subscribe(() => {
        this.RefreshLog();
      });
    }, error => {
      this.timer = interval(5000).subscribe(() => {
        this.RefreshLog();
      });
    });
  }

  ClearLog() {
    if (this.timer) {
      this.timer.unsubscribe();
      this.timer = undefined;
    }

    this.logService.clearLog().subscribe(() => {
      this.logitems = [];
      this.logitems_error = [];
      this.logitems_status = [];
      this.logitems_debug = [];
      this.timer = interval(5000).subscribe(() => {
        this.RefreshLog();
      });
    }, error => {
      this.timer = interval(5000).subscribe(() => {
        this.RefreshLog();
      });
    });
  }

  get logitemsFiltered(): Array<LogItem> {
    return this.logitems.filter(l => l.text.toLowerCase().includes(this.filterExpr.toLowerCase()));
  }

  get logitems_statusFiltered(): Array<LogItem> {
    return this.logitems_status.filter(l => l.text.toLowerCase().includes(this.filterExpr.toLowerCase()));
  }

  get logitems_errorFiltered(): Array<LogItem> {
    return this.logitems_error.filter(l => l.text.toLowerCase().includes(this.filterExpr.toLowerCase()));
  }

  get logitems_debugFiltered(): Array<LogItem> {
    return this.logitems_debug.filter(l => l.text.toLowerCase().includes(this.filterExpr.toLowerCase()));
  }

  indexFn(index, item) {
    return index;
  }

  private getLogClass(level: number): string {
    let logclass;
    if (level === LogService.LOG_STATUS) {
      logclass = 'logstatus';
    } else if (level === LogService.LOG_ERROR) {
      logclass = 'logerror';
    } else {
      logclass = 'lognorm';
    }
    return (logclass);
  }

}

interface LogItem {
  mclass: string;
  text: string;
}
