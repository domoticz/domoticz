import {Component, OnInit} from '@angular/core';
import {ApiService} from '../../_shared/_services/api.service';
import {Router} from '@angular/router';

@Component({
  selector: 'dz-restore-database',
  templateUrl: './restore-database.component.html',
  styleUrls: ['./restore-database.component.css']
})
export class RestoreDatabaseComponent implements OnInit {

  private file: File;

  constructor(private apiService: ApiService,
              private router: Router) {
  }

  ngOnInit() {
  }

  onFileChange(event: any) {
    if (event.target.files && event.target.files.length > 0) {
      this.file = event.target.files[0];
    }
  }

  onSubmit() {
    if (typeof this.file === 'undefined') {
      return;
    }
    this.uploadFile();
  }

  private uploadFile() {
    const formData = new FormData();
    formData.append('dbasefile', this.file);

    this.apiService.postFormData('restoredatabase.webem', formData).subscribe(() => {
      this.router.navigate(['/Dashboard']);
    }, error => {
      this.router.navigate(['/Dashboard']);
    });
  }

}
