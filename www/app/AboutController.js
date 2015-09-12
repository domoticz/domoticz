define(['app'], function (app) {
	app.controller('AboutController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {
		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			// The canvas element we are drawing into.
			// Credits to http://www.professorcloud.com/    
			var	$canvas = $('#canvas');
			var	$canvas2 = $('#canvas2');
			var	$canvas3 = $('#canvas3');			
			var	ctx2 = $canvas2[0].getContext('2d');
			var	ctx = $canvas[0].getContext('2d');
			var	w = $canvas[0].width;
			var h = $canvas[0].height;
			var	img = new Image();	
			
			// A puff.
			var	Puff = function(p) {				
				var	opacity,
					sy = (Math.random()*285)>>0,
					sx = (Math.random()*285)>>0;
				
				this.p = p;
						
				this.move = function(timeFac) {						
					p = this.p + 0.3 * timeFac;				
					opacity = (Math.sin(p*0.05)*0.5);						
					if(opacity <0) {
						p = opacity = 0;
						sy = (Math.random()*285)>>0;
						sx = (Math.random()*285)>>0;
					}												
					this.p = p;																			
					ctx.globalAlpha = opacity;						
					ctx.drawImage($canvas3[0], sy+p, sy+p, 285-(p*2),285-(p*2), 0,0, w, h);								
				};
			};
			
			var	puffs = [];			
			var	sortPuff = function(p1,p2) { return p1.p-p2.p; };	
			puffs.push( new Puff(0) );
			puffs.push( new Puff(20) );
			puffs.push( new Puff(40) );
			
			var	newTime, oldTime = 0, timeFac;
					
			var	loop = function()
			{								
				newTime = new Date().getTime();				
				if(oldTime === 0 ) {
					oldTime=newTime;
				}
				timeFac = (newTime-oldTime) * 0.1;
				if(timeFac>3) {timeFac=3;}
				oldTime = newTime;						
				puffs.sort(sortPuff);							
				
				for(var i=0;i<puffs.length;i++)
				{
					puffs[i].move(timeFac);	
				}					
				ctx2.drawImage( $canvas[0] ,0,0,570,570);				
				setTimeout(loop, 10 );				
			};
			// Turns out Chrome is much faster doing bitmap work if the bitmap is in an existing canvas rather
			// than an IMG, VIDEO etc. So draw the big nebula image into canvas3
			var	$canvas3 = $('#canvas3');
			var	ctx3 = $canvas3[0].getContext('2d');
			$(img).bind('load',null, function() {  ctx3.drawImage(img, 0,0, 570, 570);	loop(); });
			img.src = 'images/nebula.jpg';
		};
	} ]);
});