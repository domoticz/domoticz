import {Component, OnInit} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {mergeMap} from 'rxjs/operators';
import {HttpClient} from '@angular/common/http';
import {DomSanitizer, SafeHtml} from '@angular/platform-browser';

@Component({
  selector: 'dz-custom',
  templateUrl: './custom.component.html',
  styleUrls: ['./custom.component.css']
})
export class CustomComponent implements OnInit {

  templateContent: SafeHtml;

  constructor(private route: ActivatedRoute,
              private httpClient: HttpClient,
              private sanitizer: DomSanitizer) {
  }

  ngOnInit() {
    this.route.paramMap.pipe(
      mergeMap(paramMap => {
        const custompage = paramMap.get('custompage');
        const custompageSrc = 'templates/' + custompage + '.html';
        return this.httpClient.get(custompageSrc, {responseType: 'text'});
      })
    ).subscribe(templateContent => {
      this.templateContent = this.sanitizer.bypassSecurityTrustHtml(templateContent);
    });
  }

}
