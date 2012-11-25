// Domoticz Sensor logs
function ShowTempLog(container,id,name,backfunc)
{
  $('#modal').show();
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p><br>\n';
  
  htmlcontent+=$('#templog').html();
  $(container).html(GetBackbuttonHTMLTable(backfunc)+htmlcontent);

  $.DayChart = new Highcharts.Chart({
      chart: {
          renderTo: container + ' tempdaygraph',
          type: 'line',
          zoomType: 'xy',
          events: {
              load: function() {
                this.showLoading();
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=day",
                function(data) {
                      AddDataToTempChart(data,$.DayChart,1);
                      $.DayChart.hideLoading();
                });
              }
          }
      },
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
          text: 'Temperature Last 24 hours'
      },
      xAxis: {
          type: 'datetime'
      },
      yAxis: [{ //temp label
          labels:  {
                   formatter: function() {
                        return this.value +'\u00B0 C';
                   },
                   style: {
                      color: '#CCCC00'
                   }
          },
          title: {
              text: 'degrees Celsius',
               style: {
                  color: '#CCCC00'
               }
          }
      }, { //humidity label
          labels:  {
                   formatter: function() {
                        return this.value +'%';
                   },
                   style: {
                      color: 'limegreen'
                   }
          },
          title: {
              text: 'humidity %',
               style: {
                  color: '#00CC00'
               }
          },
          opposite: true
      }],
      tooltip: {
          formatter: function() {
            var unit = {
                        'Humidity': '%',
                        'Temperature': '\u00B0 C',
                        'Temperature_min': '\u00B0 C',
                        'Chill': '\u00B0 C',
                        'Chill_min': '\u00B0 C'
                  }[this.series.name];
                  return Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit;
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

  $.MonthChart = new Highcharts.Chart({
      chart: {
          renderTo: 'tempmonthgraph',
          type: 'spline',
          zoomType: 'xy',
          events: {
              load: function() {
                this.showLoading();
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=month",
                function(data) {
                      AddDataToTempChart(data,$.MonthChart,0);
                      $.MonthChart.hideLoading();
                });
              }
          }
      },
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
          text: 'Temperature Last Month'
      },
      xAxis: {
          type: 'datetime',
          tickInterval: 24 * 3600 * 1000
      },
      yAxis: [{ //temp label
          labels:  {
                   formatter: function() {
                        return this.value +'\u00B0 C';
                   },
                   style: {
                      color: '#CCCC00'
                   }
          },
          title: {
              text: 'degrees Celsius',
               style: {
                  color: '#CCCC00'
               }
          }
      }, { //humidity label
          labels:  {
                   formatter: function() {
                        return this.value +'%';
                   },
                   style: {
                      color: 'limegreen'
                   }
          },
          title: {
              text: 'humidity %',
               style: {
                  color: '#00CC00'
               }
          },
          opposite: true
      }],
      tooltip: {
          formatter: function() {
            var unit = {
                        'Humidity': '%',
                        'Temperature': '\u00B0 C',
                        'Temperature_min': '\u00B0 C',
                        'Chill': '\u00B0 C',
                        'Chill_min': '\u00B0 C'
                  }[this.series.name];
                  return Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit;
          }
      },
      legend: {
          enabled: true
      },
      plotOptions: {
               spline: {
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

  $.YearChart = new Highcharts.Chart({
      chart: {
          renderTo: 'tempyeargraph',
          type: 'spline',
          zoomType: 'xy',
          events: {
              load: function() {
                this.showLoading();
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=year",
                function(data) {
                      AddDataToTempChart(data,$.YearChart,0);
                      $.YearChart.hideLoading();
                });
              }
          }
      },
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
          text: 'Temperature Last Year'
      },
      xAxis: {
          type: 'datetime',
          tickInterval: 24 * 3600 * 1000
      },
      yAxis: [{ //temp label
          labels:  {
                   formatter: function() {
                        return this.value +'\u00B0 C';
                   },
                   style: {
                      color: '#CCCC00'
                   }
          },
          title: {
              text: 'degrees Celsius',
               style: {
                  color: '#CCCC00'
               }
          }
      }, { //humidity label
          labels:  {
                   formatter: function() {
                        return this.value +'%';
                   },
                   style: {
                      color: 'limegreen'
                   }
          },
          title: {
              text: 'humidity %',
               style: {
                  color: '#00CC00'
               }
          },
          opposite: true
      }],
      tooltip: {
          formatter: function() {
            var unit = {
                        'Humidity': '%',
                        'Temperature': '\u00B0 C',
                        'Temperature_min': '\u00B0 C',
                        'Chill': '\u00B0 C',
                        'Chill_min': '\u00B0 C'
                  }[this.series.name];
                  return Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit;
          }
      },
      legend: {
          enabled: true
      },
      plotOptions: {
               spline: {
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

  $('#modal').hide();
  return false;
}
