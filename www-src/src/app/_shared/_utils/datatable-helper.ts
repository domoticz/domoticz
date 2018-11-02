import { Router } from '@angular/router';

// FIXME proper declaration
declare var $: any;

export class DatatableHelper {

    /* Get the rows which are currently selected */
    public static fnGetSelected(oTableLocal: any) {
        return oTableLocal.$('tr.row_selected');
    }

    public static fixAngularLinks(linkClass: string, router: Router) {
        $(linkClass).off().on('click', function (e) {
            e.preventDefault();
            console.log('Hijacking the link to use Angular router');
            const href = this.href.replace(/^(?:\/\/|[^\/]+)*\//, '');
            router.navigate([href]);
        });
    }

}
