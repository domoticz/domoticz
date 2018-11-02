import {Component, OnInit, Inject} from '@angular/core';
import {IconService} from 'src/app/admin/customicons/icon.service';
import {Icon} from 'src/app/_shared/_models/icon';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Utils} from 'src/app/_shared/_utils/utils';

declare var bootbox: any;

@Component({
  selector: 'dz-customicons',
  templateUrl: './customicons.component.html',
  styleUrls: ['./customicons.component.css']
})
export class CustomiconsComponent implements OnInit {

  iconset: Array<Icon> = [];
  selectedIcon: Icon;
  myFile;

  constructor(
    private iconService: IconService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
    this.RefreshIconList();
  }

  private RefreshIconList(): void {
    this.iconset = [];
    this.selectedIcon = undefined;

    this.iconService.getCustomIconset().subscribe(data => {
      if (typeof data.result !== 'undefined') {
        this.iconset = data.result.sort((a, b) => a.Title.localeCompare(b.Title));
      }
    });
  }

  OnIconSelected(icon: Icon): void {
    const bWasSelected = icon.selected;
    this.iconset.forEach((item, i) => {
      item.selected = false;
    });
    icon.selected = true;
    this.selectedIcon = icon;
  }

  UploadIconSet(): void {
    const file = this.myFile;
    if (typeof file === 'undefined') {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Choose a File first!'), 2500, true);
      return;
    }
    this.iconService.uploadCustomIcon(file).subscribe(data => {
      if (data.status !== 'OK') {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Error uploading Iconset') + ': ' + data['error'], 5000, true);
      }
      this.RefreshIconList();
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Error uploading Iconset'), 5000, true);
    });
  }

  DeleteIcon() {
    bootbox.confirm(this.translationService.t('Are you sure to delete this Icon?'), (result) => {
      if (result === true) {
        this.iconService.deleteCustomIcon(this.selectedIcon.idx).subscribe(() => {
          this.RefreshIconList();
        });
      }
    });
  }

  UpdateIconTitleDescription() {
    let bValid = true;
    bValid = bValid && Utils.checkLength(this.selectedIcon.Title, 2, 100);
    bValid = bValid && Utils.checkLength(this.selectedIcon.Description, 2, 100);
    if (bValid === false) {
      this.notificationService.ShowNotify(this.translationService.t('Please enter a Name and Description!...'), 3500, true);
      return;
    }
    this.iconService.updateCustomIcon(this.selectedIcon.idx, this.selectedIcon.Title, this.selectedIcon.Description).subscribe(() => {
      this.RefreshIconList();
    });
  }

}
