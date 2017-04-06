define(['app'], function (app) {
    app.controller('TemperatureCustomLogController',
        ['$routeParams', '$scope', '$location',
            function ($routeParams, $scope, $location) {
                var ctrl = this;

                ctrl.init = function() {

                    $('#modal').show();

                    $('#bannav').i18n();
                    $('#customlog').i18n();

                    $('.datepick').datepicker({
                        dateFormat: 'yy-mm-dd',
                        onClose: function () {
                            ctrl.datePickerChanged(this);
                        }
                    });
                    $("#graphfrom").datepicker('setDate', '-7');
                    $("#graphto").datepicker('setDate', '-0');

                    $.CustomChart = $('#customlog #customgraph');
                    $.CustomChart.highcharts({
                        chart: {
                            type: 'line',
                            zoomType: 'x',
                            alignTicks: false,

                            events: {
                                load: function () {
                                }
                            }
                        },
                        colors: ['#FF99CC', '#FFCC99', '#FFFF99', '#CCFFCC', '#CCFFFF', '#99CCFF', '#CC99FF', '#FFFFFF',
                            '#9999FF', '#993366', '#FFFFCC', '#CCFFFF', '#660066', '#FF8080', '#0066CC', '#CCCCFF',
                            '#000080', '#FF00FF', '#FFFF00', '#00FFFF', '#800080', '#800000', '#008080', '#0000FF',
                            '#00C0C0', '#993300', '#333300', '#003300', '#003366', '#000080', '#333399', '#333333',
                            '#800000', '#FF6600', '#808000', '#008000', '#008080', '#0000FF', '#666699', '#808080',
                            '#FF0000', '#FF9900', '#99CC00', '#339966', '#33CCCC', '#3366FF', '#800080', '#969696',
                            '#FF00FF', '#FFCC00', '#FFFF00', '#00FF00', '#00FFFF', '#00CCFF', '#993366', '#C0C0C0'],
                        loading: {
                            hideDuration: 1000,
                            showDuration: 1000
                        },
                        credits: {
                            enabled: true,
                            href: "http://www.domoticz.com",
                            text: "Domoticz.com"
                        },
                        title: {
                            text: $.t('Custom Temperature Graph')
                        },
                        xAxis: {
                            type: 'datetime'
                        },
                        yAxis: [{ //temp label
                            labels: {
                                formatter: function () {
                                    return this.value + '\u00B0 ' + $scope.config.TempSign;
                                },
                                style: {
                                    color: 'white'
                                }
                            },
                            title: {
                                text: 'degrees Celsius',
                                style: {
                                    color: 'white'
                                }
                            },
                            showEmpty: false
                        }, { //humidity label
                            labels: {
                                formatter: function () {
                                    return this.value + '%';
                                },
                                style: {
                                    color: 'white'
                                }
                            },
                            title: {
                                text: $.t('Humidity') + ' %',
                                style: {
                                    color: 'white'
                                }
                            },
                            showEmpty: false
                        }, { //pressure label
                            labels: {
                                formatter: function () {
                                    return this.value;
                                },
                                style: {
                                    color: 'white'
                                }
                            },
                            title: {
                                text: $.t('Pressure') + ' (hPa)',
                                style: {
                                    color: 'white'
                                }
                            },
                            showEmpty: false
                        }],
                        tooltip: {
                            formatter: function () {
                                var unit = '';
                                var baseName = this.series.name.split(':')[1];
                                if (baseName == $.t("Humidity")) {
                                    unit = '%'
                                } else {
                                    unit = '\u00B0 ' + $scope.config.TempSign
                                }
                                return $.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + '<br/>' + this.series.name + ': ' + this.y + unit;
                            }
                        },
                        legend: {
                            enabled: true
                        },
                        plotOptions: {
                            line: {
                                lineWidth: 3,
                                states: {
                                    hover: {
                                        lineWidth: 3
                                    }
                                },
                                marker: {
                                    enabled: false,
                                    states: {
                                        hover: {
                                            enabled: true,
                                            symbol: 'circle',
                                            radius: 5,
                                            lineWidth: 1
                                        }
                                    }
                                }
                            }
                        }
                    });
                    ctrl.SelectGraphDevices();
                    $('#modal').hide();
                };

                ctrl.SelectGraphDevices = function()
                {
                    if (typeof $scope.mytimer != 'undefined') {
                        $interval.cancel($scope.mytimer);
                        $scope.mytimer = undefined;
                    }

                    $.ajax({
                        url: "json.htm?type=devices&filter=temp&used=true&order=Name",
                        async: false,
                        dataType: 'json',
                        success: function(data) {
                            if (typeof data.result != 'undefined') {
                                $.each(data.result, function(i,item){
                                    $("#customlog #devicecontainer").append('<input type="checkbox" class="devicecheckbox" id="'+item.idx+'" value="'+item.Name+'" onChange="AddDeviceToGraph(this)">'+item.Name+'<br />');
                                });
                            }
                        }
                    });
                };
                // Not in the controller FIXME (update above Angular way)
                AddDeviceToGraph = function(cb)
                {
                    if (cb.checked==true) {
                        $.ajax({
                            url: "json.htm?type=graph&sensor=temp&idx="+cb.id+"&range="+$("#customlog #graphfrom").val()+"T"+$("#customlog #graphto").val()+"&graphtype="+$("#customlog #combocustomgraphtype").val()+
                            "&graphTemp="+$("#customlog #graphTemp").prop("checked")+"&graphChill="+$("#customlog #graphChill").prop("checked")+"&graphHum="+$("#customlog #graphHum").prop("checked")+"&graphBaro="+$("#customlog #graphBaro").prop("checked")+"&graphDew="+$("#customlog #graphDew").prop("checked")+"&graphSet="+$("#customlog #graphSet").prop("checked"),
                            async: false,
                            dataType: 'json',
                            success: function(data) {
                                ctrl.AddMultipleDataToTempChart(data,$.CustomChart.highcharts(),$("#customlog #combocustomgraphtype").val(),cb.id,cb.value);
                            }
                        });
                    }
                    else {
                        ctrl.RemoveMultipleDataFromTempChart($.CustomChart.highcharts(),cb.id);
                    }
                };

                ctrl.datePickerChanged = function(dpicker)
                {
                    if ($("#graphfrom").val()!='') {
                        $("#customlog #graphfrom").datepicker("setDate",$("#graphfrom").val());
                    }
                    else {
                        $("#customlog #graphto").datepicker("setDate",$("#graphto").val());
                    }
                    $( "#customlog #graphfrom" ).datepicker('option', 'maxDate', $("#customlog #graphto").val());
                    $( "#customlog #graphto" ).datepicker('option', 'minDate', $("#customlog #graphfrom").val());

                    $('div[id="devicecontainer"] input:checkbox:checked').each(function() {
                        ctrl.RemoveMultipleDataFromTempChart($.CustomChart.highcharts(),$(this).attr('id'));
                        $.ajax({
                            url: "json.htm?type=graph&sensor=temp&idx="+$(this).attr('id')+"&range="+$("#customlog #graphfrom").val()+"T"+$("#customlog #graphto").val()+"&graphtype="+$("#customlog #combocustomgraphtype").val()+
                            "&graphTemp="+$("#customlog #graphTemp").prop("checked")+"&graphChill="+$("#customlog #graphChill").prop("checked")+"&graphHum="+$("#customlog #graphHum").prop("checked")+"&graphBaro="+$("#customlog #graphBaro").prop("checked")+"&graphDew="+$("#customlog #graphDew").prop("checked")+"&graphSet="+$("#customlog #graphSet").prop("checked"),
                            async: false,
                            dataType: 'json',
                            graphid: $(this).attr('id'),
                            graphval: $(this).val(),
                            success: function(data) {
                                ctrl.AddMultipleDataToTempChart(data,$.CustomChart.highcharts(),$("#customlog #combocustomgraphtype").val(),this.graphid,this.graphval);
                            }
                        });
                    });
                    return false;
                };

                ctrl.RemoveMultipleDataFromTempChart = function (chart, deviceid) {
                    hum = chart.get('humidity' + deviceid);
                    if (hum != null) {
                        hum.remove();
                    }
                    chill = chart.get('chill' + deviceid)
                    if (chill != null) {
                        chill.remove();
                    }
                    chillmin = chart.get('chillmin' + deviceid);
                    if (chillmin != null) {
                        chillmin.remove();
                    }
                    temperature = chart.get('temperature' + deviceid);
                    if (temperature != null) {
                        temperature.remove();
                    }
                    temperaturemin = chart.get('temperaturemin' + deviceid);
                    if (temperaturemin != null) {
                        temperaturemin.remove();
                    }
                    dew = chart.get('dewpoint' + deviceid);
                    if (dew != null) {
                        dew.remove();
                    }
                    baro = chart.get('baro' + deviceid);
                    if (baro != null) {
                        baro.remove();
                    }
                    setpoint = chart.get('setpoint' + deviceid);
                    if (setpoint != null) {
                        setpoint.remove();
                    }
                    setpointmin = chart.get('setpointmin' + deviceid);
                    if (setpointmin != null) {
                        setpointmin.remove();
                    }
                    setpointmax = chart.get('setpointmax' + deviceid);
                    if (setpointmax != null) {
                        setpointmax.remove();
                    }
                };

                ctrl.AddMultipleDataToTempChart = function(data,chart,isday,deviceid,devicename)
                {
                    var datatablete = [];
                    var datatabletm = [];
                    var datatablehu = [];
                    var datatablech = [];
                    var datatablecm = [];
                    var datatabledp = [];
                    var datatableba = [];
                    var datatablese = [];
                    var datatablesm = [];
                    var datatablesx = [];

                    $.each(data.result, function(i,item)
                    {
                        if (isday==1) {
                            if (typeof item.te != 'undefined') {
                                datatablete.push( [GetUTCFromString(item.d), parseFloat(item.te) ] );
                            }
                            if (typeof item.hu != 'undefined') {
                                datatablehu.push( [GetUTCFromString(item.d), parseFloat(item.hu) ] );
                            }
                            if (typeof item.ch != 'undefined') {
                                datatablech.push( [GetUTCFromString(item.d), parseFloat(item.ch) ] );
                            }
                            if (typeof item.dp != 'undefined') {
                                datatabledp.push( [GetUTCFromString(item.d), parseFloat(item.dp) ] );
                            }
                            if (typeof item.ba != 'undefined') {
                                datatableba.push( [GetUTCFromString(item.d), parseFloat(item.ba) ] );
                            }
                            if (typeof item.se != 'undefined') {
                                datatablese.push( [GetUTCFromString(item.d), parseFloat(item.se) ] );
                            }
                        } else {
                            if (typeof item.te != 'undefined') {
                                datatablete.push( [GetDateFromString(item.d), parseFloat(item.te) ] );
                                datatabletm.push( [GetDateFromString(item.d), parseFloat(item.tm) ] );
                            }
                            if (typeof item.hu != 'undefined') {
                                datatablehu.push( [GetDateFromString(item.d), parseFloat(item.hu) ] );
                            }
                            if (typeof item.ch != 'undefined') {
                                datatablech.push( [GetDateFromString(item.d), parseFloat(item.ch) ] );
                                datatablecm.push( [GetDateFromString(item.d), parseFloat(item.cm) ] );
                            }
                            if (typeof item.dp != 'undefined') {
                                datatabledp.push( [GetDateFromString(item.d), parseFloat(item.dp) ] );
                            }
                            if (typeof item.ba != 'undefined') {
                                datatableba.push( [GetDateFromString(item.d), parseFloat(item.ba) ] );
                            }
                            if (typeof item.se != 'undefined') {
                                datatablese.push( [GetDateFromString(item.d), parseFloat(item.se) ] );
                                datatablesm.push( [GetDateFromString(item.d), parseFloat(item.sm) ] );
                                datatablesx.push( [GetDateFromString(item.d), parseFloat(item.sx) ] );
                            }
                        }
                    });
                    var series;

                    if (datatablehu.length!=0)
                    {
                        chart.addSeries(
                            {
                                id: 'humidity'+deviceid,
                                name: devicename+':'+$.t('Humidity'),
                                yAxis: 1
                            }
                        );
                        series = chart.get('humidity'+deviceid);
                        series.setData(datatablehu);
                    }

                    if (datatablech.length!=0)
                    {
                        chart.addSeries(
                            {
                                id: 'chill'+deviceid,
                                name: devicename+':'+$.t('Chill'),
                                yAxis: 0
                            }
                        );
                        series = chart.get('chill'+deviceid);
                        series.setData(datatablech);

                        if (isday==0) {
                            chart.addSeries(
                                {
                                    id: 'chillmin'+deviceid,
                                    name: devicename+':'+$.t('Chill')+'_min',
                                    yAxis: 0
                                }
                            );
                            series = chart.get('chillmin'+deviceid);
                            series.setData(datatablecm);
                        }
                    }
                    if (datatablete.length!=0)
                    {
                        //Add Temperature series
                        chart.addSeries(
                            {
                                id: 'temperature'+deviceid,
                                name: devicename+':'+$.t('Temperature'),
                                yAxis: 0
                            }
                        );
                        series = chart.get('temperature'+deviceid);
                        series.setData(datatablete);
                        if (isday==0) {
                            chart.addSeries(
                                {
                                    id: 'temperaturemin'+deviceid,
                                    name: devicename+':'+$.t('Temperature')+'_min',
                                    yAxis: 0
                                }
                            );
                            series = chart.get('temperaturemin'+deviceid);
                            series.setData(datatabletm);
                        }
                    }

                    if (datatablese.length!=0)
                    {
                        //Add Temperature series
                        chart.addSeries(
                            {
                                id: 'setpoint'+deviceid,
                                name: devicename+':'+$.t('SetPoint'),
                                yAxis: 0
                            }
                        );
                        series = chart.get('setpoint'+deviceid);
                        series.setData(datatablese);
                        if (isday==0) {
                            chart.addSeries(
                                {
                                    id: 'setpointmin'+deviceid,
                                    name: devicename+':'+$.t('SetPoint')+'_min',
                                    yAxis: 0
                                }
                            );
                            series = chart.get('setpointmin'+deviceid);
                            series.setData(datatablesm);

                            chart.addSeries(
                                {
                                    id: 'setpointmax'+deviceid,
                                    name: devicename+':'+$.t('SetPoint')+'_max',
                                    yAxis: 0
                                }
                            );
                            series = chart.get('setpointmax'+deviceid);
                            series.setData(datatablesx);
                        }
                    }

                    if (datatabledp.length!=0)
                    {
                        chart.addSeries(
                            {
                                id: 'dewpoint'+deviceid,
                                name: devicename+':'+$.t('Dew Point'),
                                yAxis: 0
                            }
                        );
                        series = chart.get('dewpoint'+deviceid);
                        series.setData(datatabledp);
                    }

                    if (datatableba.length!=0)
                    {
                        chart.addSeries(
                            {
                                id: 'baro'+deviceid,
                                name: devicename+':'+$.t('Barometer'),
                                yAxis: 2
                            }
                        );
                        series = chart.get('baro'+deviceid);
                        series.setData(datatableba);
                    }

                };

                //FIXME not in controller
                ClearCustomGraph = function()
                {
                    $('div[id="devicecontainer"] input:checkbox:checked').each(function() {
                        ctrl.RemoveMultipleDataFromTempChart($.CustomChart.highcharts(),$(this).attr('id'));
                        $(this).prop("checked", false);
                    });
                };

                ctrl.init();
            }]);
});
