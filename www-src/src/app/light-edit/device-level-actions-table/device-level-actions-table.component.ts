import { Component, OnInit, ViewChild, ElementRef, AfterViewInit, Inject } from '@angular/core';
import { NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { PermissionService } from '../../_shared/_services/permission.service';
import { DeviceEditSelectorActionModalComponent } from '../device-edit-selector-action-modal/device-edit-selector-action-modal.component';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-level-actions-table',
  templateUrl: './device-level-actions-table.component.html',
  styleUrls: ['./device-level-actions-table.component.css'],
  providers: [{
    provide: NG_VALUE_ACCESSOR,
    useExisting: DeviceLevelActionsTableComponent,
    multi: true
  }]
})
export class DeviceLevelActionsTableComponent implements OnInit, AfterViewInit, ControlValueAccessor {

  @ViewChild('table', { static: false }) tableRef: ElementRef;
  table: any;

  levels: Array<any>;

  @ViewChild(DeviceEditSelectorActionModalComponent, { static: false }) modal: DeviceEditSelectorActionModalComponent;

  private propagateChange = (_: any) => { };

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private permissionService: PermissionService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const _this = this;

    this.table = $(this.tableRef.nativeElement).dataTable({
      searching: false,
      info: false,
      paging: false,
      columns: [
        { title: this.translationService.t('Level'), data: 'level', render: (level) => _this.levelRenderer(level) },
        { title: this.translationService.t('Action'), data: 'action', render: (level) => _this.actionRenderer(level) },
        { title: '', data: null, render: () => _this.actionsRenderer() }
      ]
    });

    this.table.on('click', '.js-delete', function () {
      const row = _this.table.api().row($(this).closest('tr')).data();
      _this.clearAction(row.level);
      // $scope.$apply();
    });

    this.table.on('click', '.js-update', function () {
      const row = _this.table.api().row($(this).closest('tr')).data();

      _this.modal.action = row.action;
      _this.modal.level = row.level;

      _this.modal.open();
    });
  }

  onModalAction(e: [number, string]) {
    this.setLevelAction(e[0], e[1]);
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

  private setLevelAction(index: number, action: string) {
    this.levels[index].action = action;

    this.propagateChange(this.levels);
    this.render();
  }

  private clearAction(index: number) {
    this.levels[index].action = '';

    this.propagateChange(this.levels);
    this.render();
  }

  private levelRenderer(level: number): number {
    return level * 10;
  }

  private escapeHTML(unsafe: string): string {
    return unsafe.replace(/[&<"']/g, (m) => {
      switch (m) {
        case '&':
          return '&amp;';
        case '<':
          return '&lt;';
        case '"':
          return '&quot;';
        default:
          return '&#039;';
      }
    });
  }

  private actionRenderer(action: string): string {
    return this.escapeHTML(action);
  }

  private actionsRenderer(): string {
    const actions: string[] = [];

    if (this.permissionService.hasPermission('Admin')) {
      actions.push('<img src="images/rename.png" title="' + this.translationService.t('Edit') +
        '" class="lcursor js-update" width="16" height="16"></img>');
      actions.push('<img src="images/delete.png" title="' + this.translationService.t('Clear') +
        '" class="lcursor js-delete" width="16" height="16"></img>');
    }

    return actions.join('&nbsp;');
  }

}
