import { DialogService } from '../_services/dialog.service';

// FIXME remove and use an angular dialog lib
// jQuery & jQuery UI
declare var $: any;

export abstract class DialogComponent {

    constructor(private dialogService: DialogService) { }

    init(): void {
        const _this = this;
        const dialogId = this.getDialogId();

        const dialogOptions = {
            autoOpen: false,
            width: 'auto',
            height: 'auto',
            modal: true,
            resizable: false,
            title: this.getDialogTitle(),
            buttons: this.getDialogButtons(),
            ...this.getDialogOptions(),
            open: function () {
                _this.onOpen();
            },
            close: function () {
                console.info(`close ${dialogId}`);
                _this.onClose();
                $(`#${dialogId}`).dialog('destroy').remove();
                _this.dialogService.removeDialog();
            }
        };

        $(`#${dialogId}`).dialog(dialogOptions);
    }

    public open(): void {
        $(`#${this.getDialogId()}`).dialog('open');
    }

    public close(): void {
        $(`#${this.getDialogId()}`).dialog('close');
    }

    protected abstract getDialogId(): string;
    protected abstract getDialogTitle(): string;
    protected abstract getDialogButtons(): any;

    protected onOpen(): void { }
    protected onClose(): void { }
    protected getDialogOptions(): any { return {}; }

}
