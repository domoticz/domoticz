import { Component, OnInit, AfterViewInit, ElementRef, ViewChild } from '@angular/core';

// FIXME manage with NPM+TypeScript
declare var $: any;

// FIXME do it the Angular way if possible
@Component({
  selector: 'dz-back-to-top',
  templateUrl: './back-to-top.component.html',
  styleUrls: ['./back-to-top.component.css']
})
export class BackToTopComponent implements OnInit, AfterViewInit {

  @ViewChild('element', { static: false }) elementRef: ElementRef;

  constructor() { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const _this = this;
    $(window).scroll(() => {
      if ($(window).scrollTop() <= 0) {
        $(_this.elementRef.nativeElement).fadeOut();
      } else {
        $(_this.elementRef.nativeElement).fadeIn();
      }
    });
  }

  scrollTop() {
    $('html, body').animate({ scrollTop: 0 }, 'fast');
  }

}
