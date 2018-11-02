import * as Noty from 'noty';

export class NotyHelper {

    static generate_noty(ntype: Noty.Type, ntext: string, ntimeout: number | false) {
        return new Noty({
            type: ntype,
            layout: 'topRight',
            text: ntext,
            // dismissQueue: true,
            timeout: ntimeout,
            theme: 'relax'
        }).show();
    }

}
