import {Component, Inject, OnInit} from '@angular/core';
import {DialogComponent} from 'src/app/_shared/_dialogs/dialog.component';
import {DIALOG_DATA, DialogService} from 'src/app/_shared/_services/dialog.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {HttpClient, HttpErrorResponse} from '@angular/common/http';
import {Subject} from 'rxjs';

// FIXMe proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-findlatlong-dialog',
  templateUrl: './findlatlong-dialog.component.html',
  styleUrls: ['./findlatlong-dialog.component.css']
})
export class FindlatlongDialogComponent extends DialogComponent implements OnInit {

  address: string;
  latitude: string;
  longitude: string;

  displayGeorow = true;

  whenOk: Subject<[string, string]> = new Subject<[string, string]>();

  constructor(
    dialogService: DialogService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    @Inject(DIALOG_DATA) data: any,
    private notificationService: NotificationService,
    private httpClient: HttpClient) {

    super(dialogService);
  }

  ngOnInit() {
    if ('geolocation' in navigator) {
      // Nothing to do
    } else {
      this.displayGeorow = false;
    }
  }

  protected getDialogOptions(): any {
    return {
      width: 480,
      height: 280
    };
  }

  protected getDialogId(): string {
    return 'dialog-findlatlong';
  }

  protected getDialogTitle(): string {
    return 'Find your Geo location';
  }

  protected getDialogButtons() {
    return {
      'OK': () => {
        let bValid = true;
        bValid = bValid && Utils.checkLength(this.latitude, 3, 100);
        bValid = bValid && Utils.checkLength(this.longitude, 3, 100);
        if (bValid) {
          this.whenOk.next([this.latitude, this.longitude]);
          this.close();
        } else {
          bootbox.alert(this.translationService.t('Please enter a Latitude and Longitude!...'), 3500, true);
        }
      },
      Cancel: () => {
        this.close();
      }
    };
  }

  getLatLong() {
    const address = this.address;
    if (address === '') {
      bootbox.alert(this.translationService.t('Please enter a Address to search for!...'), 3500, true);
      return false;
    }

    this.httpClient.get('https://www.mapquestapi.com/geocoding/v1/address', {
      params: {
        key: 'XN5Eyt9GjLaRPG6T2if7VtUueRLckR8b',
        inFormat: 'kvp',
        outFormat: 'json',
        thumbMaps: 'false',
        location: address
      }
    }).subscribe(data => {
      let bIsOK = false;
      if (data.hasOwnProperty('results')) {
        if (data['results'][0]['locations'].length > 0) {
          this.latitude = data['results'][0]['locations'][0]['displayLatLng']['lat'];
          this.longitude = data['results'][0]['locations'][0]['displayLatLng']['lng']; // .toFixed(6)
          bIsOK = true;
        }
      }
      if (!bIsOK) {
        bootbox.alert(this.translationService.t('Geocode was not successful for the following reason') + ': Invalid/No data returned!');
      }
    }, (error: HttpErrorResponse) => {
      bootbox.alert(this.translationService.t('Geocode was not successful for the following reason') + ': ' + error.statusText);
    });

    return false;
  }

  detect() {
    navigator.geolocation.getCurrentPosition((location: Position) => {
      this.latitude = location.coords.latitude.toString();
      this.longitude = location.coords.longitude.toString();
    });
  }

}
