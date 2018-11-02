import {Component, OnInit} from '@angular/core';
import {ActivatedRoute, NavigationEnd, Router} from '@angular/router';

@Component({
  selector: 'dz-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit {

  layout = true;

  currentyear: number = new Date().getFullYear();

  constructor(private router: Router,
              private activatedRoute: ActivatedRoute) {
  }

  ngOnInit() {
    // Some specific pages (components) don't want to display the usual layout (header...)
    // To do so, we use route data attributes
    this.router.events.subscribe(event => {
      if (event instanceof NavigationEnd) {
        this.layout = this.activatedRoute.firstChild.snapshot.data.layout !== false;
      }
    });
  }

}
