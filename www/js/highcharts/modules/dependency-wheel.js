/*
  Highcharts JS v7.1.2 (2019-06-03)

 Dependency wheel module

 (c) 2010-2018 Torstein Honsi

 License: www.highcharts.com/license
*/
(function(a){"object"===typeof module&&module.exports?(a["default"]=a,module.exports=a):"function"===typeof define&&define.amd?define("highcharts/modules/dependency-wheel",["highcharts","highcharts/modules/sankey"],function(d){a(d);a.Highcharts=d;return a}):a("undefined"!==typeof Highcharts?Highcharts:void 0)})(function(a){function d(a,d,n,b){a.hasOwnProperty(d)||(a[d]=b.apply(null,n))}a=a?a._modules:{};d(a,"modules/dependency-wheel.src.js",[a["parts/Globals.js"]],function(a){var d=a.seriesTypes.sankey.prototype;
a.seriesType("dependencywheel","sankey",{center:[null,null],curveFactor:.6,startAngle:0},{orderNodes:!1,getCenter:a.seriesTypes.pie.prototype.getCenter,createNodeColumns:function(){var a=[this.createNodeColumn()];this.nodes.forEach(function(b){b.column=0;a[0].push(b)});return a},getNodePadding:function(){return this.options.nodePadding/Math.PI},createNode:function(a){var b=d.createNode.call(this,a);b.index=this.nodes.length-1;b.getSum=function(){return b.linksFrom.concat(b.linksTo).reduce(function(a,
b){return a+b.weight},0)};b.offset=function(a){function k(a){return a.fromNode===b?a.toNode:a.fromNode}var g=0,c,f=b.linksFrom.concat(b.linksTo),h;f.sort(function(a,b){return k(a).index-k(b).index});for(c=0;c<f.length;c++)if(k(f[c]).index>b.index){f=f.slice(0,c).reverse().concat(f.slice(c).reverse());h=!0;break}h||f.reverse();for(c=0;c<f.length;c++){if(f[c]===a)return g;g+=f[c].weight}};return b},translate:function(){var n=this.options,b=2*Math.PI/(this.chart.plotHeight+this.getNodePadding()),h=this.getCenter(),
k=(n.startAngle-90)*a.deg2rad;d.translate.call(this);this.nodeColumns[0].forEach(function(a){var c=a.shapeArgs,f=h[0],d=h[1],g=h[2]/2,l=g-n.nodeWidth,m=k+b*c.y,c=k+b*(c.y+c.height);a.angle=m+(c-m)/2;a.shapeType="arc";a.shapeArgs={x:f,y:d,r:g,innerR:l,start:m,end:c};a.dlBox={x:f+Math.cos((m+c)/2)*(g+l)/2,y:d+Math.sin((m+c)/2)*(g+l)/2,width:1,height:1};a.linksFrom.forEach(function(a){var c,e=a.linkBase.map(function(e,h){e*=b;var g=Math.cos(k+e)*(l+1),m=Math.sin(k+e)*(l+1),p=n.curveFactor;c=Math.abs(a.linkBase[3-
h]*b-e);c>Math.PI&&(c=2*Math.PI-c);c*=l;c<l&&(p*=c/l);return{x:f+g,y:d+m,cpX:f+(1-p)*g,cpY:d+(1-p)*m}});a.shapeArgs={d:["M",e[0].x,e[0].y,"A",l,l,0,0,1,e[1].x,e[1].y,"C",e[1].cpX,e[1].cpY,e[2].cpX,e[2].cpY,e[2].x,e[2].y,"A",l,l,0,0,1,e[3].x,e[3].y,"C",e[3].cpX,e[3].cpY,e[0].cpX,e[0].cpY,e[0].x,e[0].y]}})})},animate:function(d){if(!d){var b=a.animObject(this.options.animation).duration/2/this.nodes.length;this.nodes.forEach(function(a,d){var h=a.graphic;h&&(h.attr({opacity:0}),setTimeout(function(){h.animate({opacity:1},
{duration:b})},b*d))},this);this.points.forEach(function(a){var b=a.graphic;!a.isNode&&b&&b.attr({opacity:0}).animate({opacity:1},this.options.animation)},this);this.animate=null}}},{setState:a.NodesMixin.setNodeState,getDataLabelPath:function(a){var b=this.series.chart.renderer,d=this.shapeArgs,k=0>this.angle||this.angle>Math.PI,g=d.start,c=d.end;this.dataLabelPath||(this.dataLabelPath=b.arc({open:!0}).add(a));this.dataLabelPath.attr({x:d.x,y:d.y,r:d.r+(this.dataLabel.options.distance||0),start:k?
g:c,end:k?c:g,clockwise:+k});return this.dataLabelPath},isValid:function(){return!0}})});d(a,"masters/modules/dependency-wheel.src.js",[],function(){})});

