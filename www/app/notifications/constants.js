define(function () {
    return {
        typeNameByTypeMap: {
            'T': $.t('Temperature'),
            'D': $.t('Dew Point'),
            'H': $.t('Humidity'),
            'R': $.t('Rain'),
            'W': $.t('Wind'),
            'U': $.t('UV'),
            'M': $.t('Usage'),
            'B': $.t('Baro'),
            'S': $.t('Switch On'),
            'O': $.t('Switch Off'),
            'E': $.t('Today'),
            'G': $.t('Today'),
            'C': $.t('Today'),
            '1': $.t('Ampere 1'),
            '2': $.t('Ampere 2'),
            '3': $.t('Ampere 3'),
            'P': $.t('Percentage'),
            'V': $.t('Play Video'),
            'A': $.t('Play Audio'),
            'X': $.t('View Photo'),
            'Y': $.t('Pause Stream'),
            'Q': $.t('Stop Stream'),
            'a': $.t('Play Stream'),
            'J': $.t('Last Update')
        },
        unitByTypeMap: {
            'T': '° ' + $.myglobals.tempsign,
            'D': '° ' + $.myglobals.tempsign,
            'H': '%',
            'R': 'mm',
            'W': $.myglobals.windsign,
            'U': 'UVI',
            'B': 'hPa',
            'E': 'kWh',
            'G': 'm3',
            'C': 'cnt',
            '1': 'A',
            '2': 'A',
            '3': 'A',
            'P': '%',
            'J': 'min'
        },
        whenByTypeMap: {
            'S': $.t('On'),
            'O': $.t('Off'),
            'D': $.t('Dew Point'),
            'V': $.t('Video'),
            'A': $.t('Audio'),
            'X': $.t('Photo'),
            'Y': $.t('Paused'),
            'Q': $.t('Stopped'),
            'a': $.t('Playing')
        },
        whenByConditionMap: {
            '>': $.t('Greater'),
            '>=': $.t('Greater or Equal'),
            '=': $.t('Equal'),
            '!=': $.t('Not Equal'),
            '<=': $.t('Less or Equal'),
            '<': $.t('Less')
        },
        priorities: [
            $.t('Very Low'),
            $.t('Moderate'),
            $.t('Normal'),
            $.t('High'),
            $.t('Emergency')
        ]
    };
});