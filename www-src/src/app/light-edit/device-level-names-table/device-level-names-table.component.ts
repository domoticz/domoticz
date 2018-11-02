import { Component, OnInit, ViewChild, ElementRef, AfterViewInit, Inject } from '@angular/core';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { ControlValueAccessor, NG_VALUE_ACCESSOR } from '@angular/forms';
import { PermissionService } from '../../_shared/_services/permission.service';
import { DeviceLevelRenameModalComponent } from '../device-level-rename-modal/device-level-rename-modal.component';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-level-names-table',
  templateUrl: './device-level-names-table.component.html',
  styleUrls: ['./device-level-names-table.component.css'],
  providers: [{
    provide: NG_VALUE_ACCESSOR,
    useExisting: DeviceLevelNamesTableComponent,
    multi: true
  }]
})
export class DeviceLevelNamesTableComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  newLevelName;
  levels: Array<any> = [];

  @ViewChild('table', { static: false }) tableRef: ElementRef;
  table: any;

  @ViewChild(DeviceLevelRenameModalComponent, { static: false }) renameModal: DeviceLevelRenameModalComponent;

  private propagateChange = (_: any) => { };

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private permissionService: PermissionService,
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const _this = this;

    this.table = $(this.tableRef.nativeElement).dataTable({
      info: false,
      searching: false,
      paging: false,
      columns: [
        { title: this.translationService.t('Level'), data: 'level', render: (level) => _this.levelRenderer(level) },
        { title: this.translationService.t('Level name'), data: 'name' },
        { title: this.translationService.t('Order'), data: 'level', render: (level) => _this.orderRenderer(level) },
        { title: '', data: 'level', render: (level) => _this.actionsRenderer(level) },
      ],
    });

    this.table.on('click', '.js-order-up', function () {
      const row = _this.table.api().row($(this).closest('tr')).data();
      _this.switchLevels(row.level, row.level - 1);
      // $scope.$apply();
    });

    this.table.on('click', '.js-order-down', function () {
      const row = _this.table.api().row($(this).closest('tr')).data();
      _this.switchLevels(row.level, row.level + 1);
      // $scope.$apply();
    });

    this.table.on('click', '.js-update', function () {
      const row = _this.table.api().row($(this).closest('tr')).data();

      _this.renameModal.name = row.name;
      _this.renameModal.level = row.level;

      _this.renameModal.open();
    });

    this.table.on('click', '.js-delete', function () {
      const row = _this.table.api().row($(this).closest('tr')).data();
      _this.deleteLevel(row.level);
      // $scope.$apply();
    });
  }

  onRenameModal(e: [number, string]) {
    this.setLevelName(e[0], e[1]);
  }

  writeValue(value: any): void {
    if (value !== undefined && value !== null) {
      this.levels = value;
      this.render();
    }
  }

  registerOnChange(fn: any): void {
    this.propagateChange = fn;
  }

  registerOnTouched(fn: any): void {
  }

  private render() {
    const items = this.levels;

    this.table.api().rows().clear();
    this.table.api().rows.add(items).draw();
  }

  private setLevelName(index: number, name: string) {
    this.levels[index].name = name;

    this.propagateChange(this.levels);
    this.render();
  }

  isAddLevelAvailable(): boolean {
    const isNotEmpty = !!this.newLevelName;
    const isUnique = !this.levels.find((item) => {
      return item.name === this.newLevelName;
    });

    return isNotEmpty && isUnique;
  }

  addLevel() {
    const levelName = this.newLevelName.replace(/[:;|<>]/g, '').trim();

    const newItem = {
      level: this.levels.length,
      name: levelName,
      action: ''
    };

    this.levels = this.levels.concat(newItem);
    this.propagateChange(this.levels);
    this.render();
    this.newLevelName = '';
  }

  private switchLevels(indexA: number, indexB: number): void {
    const levelAName = this.levels[indexA].name;
    const levelBName = this.levels[indexB].name;

    this.levels[indexA].name = levelBName;
    this.levels[indexB].name = levelAName;

    this.propagateChange(this.levels);
    this.render();
  }

  private deleteLevel(index: number): void {
    this.levels = this.levels
      .filter((item) => {
        return item.level !== index;
      })
      .map((item, i) => {
        return Object.assign({}, item, { level: i });
      });

    this.propagateChange(this.levels);
    this.render();
  }

  private levelRenderer(level: number): number {
    return level * 10;
  }

  private orderRenderer(level: number): string {
    const images: Array<string> = [];

    if (level < (this.levels.length - 1)) {
      images.push('<img src="images/down.png" class="lcursor js-order-down" width="16" height="16"></img>');
    } else {
      images.push('<img src="images/empty16.png" width="16" height="16"></img>');
    }

    if (level > 0) {
      images.push('<img src="images/up.png" class="lcursor js-order-up" width="16" height="16"></img>');
    }

    return images.join('&nbsp;');
  }

  private actionsRenderer(level: number) {
    const actions: Array<string> = [];

    if (this.permissionService.hasPermission('Admin')) {
      actions.push('<img src="images/rename.png" title="' + this.translationService.t('Rename') +
        '" class="lcursor js-update" width="16" height="16"></img>');
      actions.push('<img src="images/delete.png" title="' + this.translationService.t('Delete') +
        '" class="lcursor js-delete" width="16" height="16"></img>');
    }

    return actions.join('&nbsp;');
  }

}
