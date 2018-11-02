
export class ReportHelpers {

    static addTrendData<T>(items: Array<T>, key: string): Array<T & {trend: string}> {
        return items.map((item: T, index: number) => {
            let trend = 'equal';

            if (index > 0 && item[key] > items[index - 1][key]) {
                trend = 'up';
            }

            if (index > 0 && item[key] < items[index - 1][key]) {
                trend = 'down';
            }

            return Object.assign({}, item, {
                trend: trend
            });
        });
    }

}
