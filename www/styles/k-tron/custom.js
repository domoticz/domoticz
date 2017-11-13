/*  

Dear themer 

You can use this file to add your own javascript functions to your theme. 

Some useful things to get you started:

1.
To find out which classes are available to you, have a look at the page on the WIKI about theming.
http://domoticz.com/wiki/How_to_theme_Domoticz


2.
Here are some snippets you may find useful:

// This would target all the air quality items onthe dashboard in the utilities section:
$('body.columns3 section#dashUtility .AirQuality').each(function(){
    console.log("hello air quality sensor");
}

// check if the user is on a mobile device:
if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
    console.log("User has chosen the mobile display as the dashboard, or is on a mobile phone.");
}

// Avoiding grouped items (if this feature has made it into Domoticz). This code does a double check: 
- only select the first three items of each section
- don't select items called 'bandleader' (grouped items are called 'bands' in the code).
if ($("section#"+ section + " #"+theid).is($("#dashcontent h2 + div.divider .span4:nth-child(-n+3)")) && !$("section#"+ section + " #" + theid + " div").hasClass('bandleader')) { 

*/



$( document ).ready(function() {
    console.log( "THEME JS body ready" );  
    
    //Place you JS here. It may be smart to run your code with a small delay, or let it respond to a finished ajax call. See the code below for examples of this.
    
    
    
    // Making changes to Domoticz html in order for the theme to function better. Many changes to the Domoticz HTML output have been proposed, but few made it in. This add those changes via JS.
    window.setTimeout(updateMainPage,3500);    
    window.setTimeout(addSpans,3000);
    
    $( "#appnavbar > li" ).click(function() {
        window.setTimeout(updateMainPage,500);
    });
    
    $( document ).ajaxSuccess(function( event, xhr, settings ) {
        console.log( "THEME JS - domoticz received ajax" );
        
        if ( settings.url.startsWith("json.htm?type=command&param=switchdeviceorder") ) {
            window.setTimeout(updateMainPage,500);
        }else{
            window.setTimeout(addSpans,200);
        }
        
    });
    

    
});



// adds extra html to make theming work better.
function addSpans()
{
    $('td[id="name"]:not(:has(span))').each(function(index) {
        $(this).wrapInner('<span><\/span>');
    });

    $('td[id="bigtext"]:not(:has(span))').each(function(index) {
        $(this).wrapInner('<span><\/span>');
    });
}

// makes changes to HTML on each new loaded page.
function updateMainPage()
{
    
    // simply the dashboard
    
    //$('.dashCategory section').each(function(index) {
    //    $(this).find(".span4").wrapAll( "<div class='row divider' />"); 
    //});
    if($('section.dashCategory').length > 0){
        // on the dashboard
        $('section.dashCategory').each(function(index) {
            console.log( "IN SECTION" );
            $(this).find('div.divider > div.span4').unwrap();
            $(this).find('div.span4').wrapAll( "<div class='row divider' />");
        });
        
        // addDataviz(); //disabled for now to keep this theme a bit simpler.
        
    }else{
        // not on the dashboard
        $('div.divider:not(div[id="weatherwidgets"]) > div.item').unwrap();
        $( ".container > div.item" ).wrapAll( "<div class='row divider' />");
    }

    addSpans();

}



/* Adds Data Visualisations to the first three items of each section. They will only be shown if highlights are enabled.  A feature like this could also be inside a theme folder, perhaps as a custom.js file that gets loaded per theme. I tried loading this form an external file, but couldn't get that to work. */
addDataviz = function () {
    //if ($scope.config.DashboardType == 0) { // Only do all this on normal display.
        console.log("Dataviz is enabled");
        setTimeout(function () { // small delay to make sure the main html has been generated, and to lower the burden on the system.

            /* temperature */
            $('#dashTemperature .item').each(function () {
                var theid = '' + $(this).parent().attr('id');
                generateDataviz("dashTemperature", "graph", "temp", "te", theid, "day");
            });

            /*  general */
            $('#dashUtility .item').each(function () {
                var theid = '' + $(this).parent().attr('id');
                generateDataviz("dashUtility", "graph", "Percentage", "any", theid, "day");
            });

            /*  Lux */
            $('body.columns3 section#dashUtility > .divider:first-of-type .Lux').each(function () {
                var theid = '' + $(this).parent().attr('id');
                generateDataviz("dashUtility", "graph", "counter", "lux", theid, "day");
            });

            /*  Co2 */
            $('body.columns3 section#dashUtility > .divider:first-of-type .AirQuality').each(function () {
                var theid = '' + $(this).parent().attr('id');
                generateDataviz("dashUtility", "graph", "counter", "any", theid, "day");
            });

            /*  Energy */
            $('body.columns3 section#dashUtility > .divider:first-of-type .Energy').each(function () {
                var theid = '' + $(this).parent().attr('id');
                generateDataviz("dashUtility", "graph", "counter", "any", theid, "day");
            });

        }, 3250);
        // Create new  timer if it does not already exist

        if (typeof datavizTimer === 'undefined' || datavizTimer === null) {
            console.log("Creating new Dataviz timer"); // <-- just for debugging
            datavizTimer = setInterval(addDataviz, 600000); // updates the dataviz every 10 minutes.
        }else{console.log("interval timer already existed");}
    //}
}

generateDataviz = function (section, type, sensor, thekey, theid, range) {
    console.log('generate dataviz has been called');
    var agent = '' + theid;
    var n = agent.lastIndexOf('_');
    var idx = agent.substring(n + 1);
    if ($("section#" + section + " #" + theid).is($("#dashcontent h2 + div.divider .span4:nth-child(-n+3)")) && !$("section#" + section + " #" + theid + " div").hasClass('bandleader')) {  // avoid doing this for grouped items
        console.log('making dataviz for item: ' + agent);
        var urltoload = 'json.htm?type=' + type + '&sensor=' + sensor + '&idx=' + idx + '&range=' + range;
        $('<td class="dataviz"><div></div></td>').insertBefore("#" + theid + " .status");
        var showData = $('section#' + section + ' #' + theid).find('.dataviz > div');
        var datavizArray = [];
        $.getJSON(urltoload, function (data) {
            //console.log( "Dataviz - JSON load success" );
            if (typeof data.result != "undefined") {
                var modulo = 1
                if (data.result.length > 100) { modulo = 2; }
                if (data.result.length > 200) { modulo = 4; }
                if (data.result.length > 300) { modulo = 6; }
                if (data.result.length > 400) { modulo = 8; }
                if (data.result.length > 500) { modulo = 10; }
                if (data.result.length > 600) { modulo = 16; }
                for (var i in data.result) {
                    var key = i;
                    var val = data.result[i];
                    if ((i % modulo) == 0) { // this prunes and this limits the amount of datapoints, to make it all less heavy on the browser.
                        for (var j in val) {
                            var readytobreak = 0;
                            var key2 = j;
                            var val2 = val[j];
                            if (thekey != 'any') {
                                if (key2 == thekey) {
                                    //console.log("adding data");
                                    var addme = Math.round(val2 * 10) / 10;
                                    datavizArray.push(addme);
                                }
                            } else if (key2 != 'd') {
                                var addme = Math.round(val2 * 10) / 10;
                                datavizArray.push(addme);
                                readytobreak = 0
                            }
                            if (readytobreak == 1) { break; } // if grabbing "any" value, then break after the first one.
                        }
                    }
                }
                // Attach the datavizualisation
                if (datavizArray.length > 0) {
                    showData.highcharts({
                        chart: {
                            type: 'line',
                            backgroundColor: 'transparent',
                            plotBorderWidth: null,
                            marginTop: 0,
                            height: 40,
                            marginBottom: 0,
                            marginLeft: 0,
                            plotShadow: false,
                            borderWidth: 0,
                            plotBorderWidth: 0,
                            marginRight: 0
                        },
                        tooltip: {
                            userHTML: true,
                            style: {
                                padding: 5,
                                width: 100,
                                height: 30,
                                backgroundColor: '#FCFFC5',
                                borderColor: 'black',
                                borderRadius: 10,
                                borderWidth: 3,
                            },
                            formatter: function () {
                                return '<b>' + this.y + '</b> (' + range + ')';// 
                            },
                            height: 30,
                            width: 30
                        },
                        title: {
                            text: ''
                        },
                        xAxis: {
                            gridLineWidth: 0,
                            minorGridLineWidth: 0,
                            enabled: false,
                            showEmpty: false,
                        },
                        yAxis: {
                            gridLineWidth: 0,
                            minorGridLineWidth: 0,
                            title: {
                                text: ''
                            },
                            showEmpty: true,
                            enabled: true
                        },
                        credits: {
                            enabled: false
                        },
                        legend: {
                            enabled: false
                        },
                        plotOptions: {
                            line: {
                                lineWidth: 1.5,
                                lineColor: '#cccccc',
                            },
                            showInLegend: true,
                            tooltip: {
                            }
                        },
                        exporting: {
                            buttons: {
                                contextButton: {
                                    enabled: false
                                }
                            }
                        },
                        series: [{
                            marker: {
                                enabled: false
                            },
                            animation: true,
                            name: '24h',
                            data: datavizArray //[19.5,20,17]  
                        }]
                    });
                }
            }
        });
    }else{console.log("dataviz not triggered")}
};







/* This is currently unused code to allow users to change the theme through settings in the interface.

$( document ).ready(function() {
    console.log( "THEME JS body ready" );  
    
    // VARIABLES
    
    var showversionnumber = 1; // shows the version number above the logo 
    //var dashboardmergeitemswithsamename = 1; // combines items with the same name into one item 
    //var dashboardshowdatavizualisations = 1; // Adds 24h data visualisation on some elements 
    //var dashboardmovesun = 1; // Moves the sun-up indicator to the footer. 
    //var dashboardshowlastupdate = 1; // Displays the 'last update' time on the dashboard. 
    var dashboardvariableitemsperrow = 1; // place more (or less) items on one row. 
    var dashboardmasonry = 0; // makes different sized items fit together better.
    var dashboardthinneritemsonwidescreens = 0; // Only for wide screens; item-options move next to the name.
    var dashboardhighlights = 1; // This adds bigger items at the top of each dashboard category.
    var dashboardhighlightsseethrough = 1; // makes all dashboard items semi-transparent.
    //var dashboardverticalcolumns = 1; // Turns dashboard into vertical columns.
    var navigationmainsidebar = 1; // This transforms the main menu to a vertical menu on the left.
    //var navigationsettingssidemenu= 1; // This transforms the settings-tabs into a sidebar on the left.
    var navigationsettingsfixedtabs = 0; // makes settings-tabs at the top stay on screen.
    var centerpopups = 1; //centers pop-up items in the middle of the screen. 
    var extrasandanimations = 1; // Adds visual extras.



    if (showversionnumber == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/show_version_number.css'
            });  
    }



    if (typeof $rootScope.config.theme_MergeUtilities != 'undefined') {
        if ($rootScope.config.theme_MergeUtilities == 1) {
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/dashboard_merge_items_with_same_name.css'
                });  
        }
    }else{
        console.log("THEME JS - VARIABLE NOT SET");
    } 
    
    
    
    if (typeof $rootScope.config.theme_MergeUtilities != 'undefined') {
        if ($rootScope.config.theme_MergeUtilities == 1) {
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/dashboard_merge_items_with_same_name.css'
                });  
        }
    }else{
        console.log("THEME JS - VARIABLE NOT SET");
    }



    if (typeof $rootScope.config.theme_ShowDataVisualisations != 'undefined') {
        if ($rootScope.config.theme_ShowDataVisualisations == 1) {
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/dashboard_show_data_vizualisations.css'
                });
        }
    }



    if (typeof $rootScope.config.theme_SunriseTimesAtTop != 'undefined') {
        if (typeof $rootScope.config.theme_SunriseTimesAtTop == 1) {
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/dashboard_move_sun.css'
                }); 
        }
    }    



    if (typeof $rootScope.config.theme_ShowLastUpdateTime != 'undefined') {
        if (typeof $rootScope.config.theme_ShowLastUpdateTime == 1) {    
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/dashboard_show_last_update.css'
                });
        }
    }



    if (dashboardvariableitemsperrow == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/dashboard_variable_items_per_row.css'
            });  
    }    

    if (dashboardmasonry == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/dashboard_masonry.css'
            });  
    }    
    
    if (dashboardthinneritemsonwidescreens == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/dashboard_thinner_items_on_wide_screens.css'
            });  
    }        
    
    if (dashboardhighlights == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/dashboard_highlights.css'
            });  
    }



    if (dashboardhighlightsseethrough == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/dashboard_highlights_see-through.css'
            });  
    }    
    
    
    
    if (typeof $rootScope.config.theme_VerticalColumnsOnDashboard != 'undefined') {  
        if (typeof $rootScope.config.theme_VerticalColumnsOnDashboard == 1) {  
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/dashboard_vertical_columns.css'
                });
        }
    }



    if (navigationmainsidebar == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/navigation_main_sidebar.css'
            });  
    }



    if (typeof $rootScope.config.theme_VerticalMenuForSettings != 'undefined') {
        if (typeof $rootScope.config.theme_VerticalMenuForSettings == 1) {    
            $('<link>')
                .appendTo('head')
                .attr({
                    type: 'text/css', 
                    rel: 'stylesheet',
                    href: 'acttheme/navigation_settings_sidemenu.css'
                });
        }
    }

    if (navigationsettingsfixedtabs == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/navigation_settings_fixed_tabs.css'
            });  
    }
    
    if (centerpopups == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/center-popups.css'
            });  
    }    
    
    if (extrasandanimations == 1) {
        $('<link>')
            .appendTo('head')
            .attr({
                type: 'text/css', 
                rel: 'stylesheet',
                href: 'acttheme/extras_and_animations.css'
            });  
    }        
    

});

*/