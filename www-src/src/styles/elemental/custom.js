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