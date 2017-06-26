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





/*
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