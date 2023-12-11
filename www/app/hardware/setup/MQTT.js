define(['app'], function (app) {
    app.component('mqttHardware', {
        bindings: {
            hardware: '<'
        },
        templateUrl: 'app/hardware/setup/MQTT.html',
        controller: Controller
    });

    function Controller() {
        var $ctrl = this;

        $ctrl.$onInit = function () {
            $.devIdx = $ctrl.hardware.idx;

			var defaultOptions = {
				availableListPosition: 'left',
				splitRatio: 0.5,
				moveEffect: 'blind',
				moveEffectOptions: { direction: 'vertical' },
				moveEffectSpeed: 'fast'
			};

			$("#mqttdevicestable .multiselect").multiselect(defaultOptions);
			
			//Get devices
			$("#hardwarecontent #mqttdevicestable #devices").html("");
			$.ajax({
				url: "json.htm?type=command&param=devices_list",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var option = $('<option />');
							option.attr('value', item.idx).text(item.name_type);
							$("#hardwarecontent #mqttdevicestable #devices").append(option);
						});
					}
				}
			});

			ShowDevices();

            $('#hardwarecontent #idx').val($ctrl.hardware.idx);
        };

		ShowDevices = function () {
			$.ajax({
				url: "json.htm?type=command&param=getsharedmqttdevices&idx=" + $.devIdx,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var selstr = '#mqttdevicestable .multiselect option[value="' + item.DeviceRowIdx + '"]';
							$(selstr).attr("selected", "selected");
						});
					}
				}
			});
		}
		SaveMQTTDevices = function () {
			var selecteddevices = $("#mqttdevicestable .multiselect option:selected").map(function () { return this.value }).get().join(";");
			$.ajax({
				url: "json.htm?type=command&param=setsharedmqttdevices&idx=" + $.devIdx + "&devices=" + encodeURIComponent(selecteddevices),
				async: false,
				dataType: 'json',
				success: function (data) {
					ShowDevices();
					ShowNotify($.t('Settings saved'), 1000);
				},
				error: function () {
					ShowNotify($.t('Problem saving settings!'), 2500, true);
				}
			});
		}
		ClearMQTTDevices = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
				$.ajax({
					url: "json.htm?type=command&param=clearsharedmqttdevices&idx=" + $.devIdx,
					async: false,
					dataType: 'json',
					success: function (data) {
						var selected = $("#mqttdevicestable .multiselect option:selected");
						for(var i = 0; i < selected.length; i++){
							selected[i].selected = false;
						}			
						$("#mqttdevicestable .multiselect").multiselect('refresh');
						ShowDevices();
					},
					error: function () {
					}
				});
			});
		}
    }
});
