/*
//    (c)  Kendel Boul - 2014
*/

String.prototype.setCharAt = function (idx, chr) {
    if (idx > this.length - 1) {
        return this.toString();
    } else {
        return this.substr(0, idx) + chr + this.substr(idx + 1);
    }
};

function getSVGnode() {
    var divFloors = $(".imageparent");
    if (divFloors.length == 0) return;
    var i = 0;
    while (i < divFloors[0].children.length) {
        if (divFloors[0].children[i].tagName == 'svg') return divFloors[0].children[i];
        i++;
    }
    return;
}

function makeSVGnode(tag, attrs, text, title) {
    var el = document.createElementNS('http://www.w3.org/2000/svg', tag);
    for (var k in attrs)
        if (k == "xlink:href") el.setAttributeNS('http://www.w3.org/1999/xlink', 'href', attrs[k]);
        else el.setAttribute(k, attrs[k]);
    if ((typeof text != 'undefined') && (text.length != 0)) {
        el.appendChild(document.createTextNode(text));
    }
    if ((typeof title != 'undefined') && (title.length != 0)) {
        var elTitle = document.createElementNS('http://www.w3.org/2000/svg', 'title');
        if (title.length != 0) elTitle.appendChild(document.createTextNode(title));
        el.appendChild(elTitle);
    }
    return el;
}

function makeSVGmultiline(attrs, text, title, maxX, minY, incY, separator) {
    // no wrap parameters
    if ((typeof maxX == 'undefined') || (typeof minY == 'undefined') || (typeof incY == 'undefined')) {
        return makeSVGnode('text', attrs, text, title);
    }
    // no text to wrap
    if ((typeof text == 'undefined') || (text.length == 0)) {
        return makeSVGnode('text', attrs, text, title);
    }

    var elText = makeSVGnode('text', attrs, '', title);
    var svg = getSVGnode();
    svg.appendChild(elText);
    var elSpan = makeSVGnode('tspan', {}, text);
    elText.appendChild(elSpan);
    if (elSpan.getComputedTextLength() <= maxX) {
        svg.removeChild(svg.childNodes[svg.childNodes.length - 1]);
        return elText;
    }
    // now things get interesting
    elText.setAttribute('y', minY);
    elSpan.firstChild.data = '';

    if (typeof separator == 'undefined') separator = ', ';
    var phrase = text.split(separator);
    if (phrase == text) {
        separator = ' ';
        phrase = text.split(separator);
    }
    var len = 0;
    for (var i = 0; i < phrase.length; i++) {
        if (separator == "<br />") {
            elSpan = makeSVGnode('tspan', {}, text);
            elText.appendChild(elSpan);
            elSpan.setAttribute('x', 0);
            elSpan.setAttribute('dy', incY);
            elSpan.firstChild.data = phrase[i];
        }
        else {
            if (i != 0) {
                elSpan.firstChild.data += separator;
            }
            len = elSpan.firstChild.data.length;
            elSpan.firstChild.data += phrase[i];

            if (elSpan.getComputedTextLength() > maxX) {
                elSpan.firstChild.data = elSpan.firstChild.data.slice(0, len).trim();    // Remove added word
                elSpan = makeSVGnode('tspan', {}, text);
                elText.appendChild(elSpan);
                elSpan.setAttribute('x', 0);
                elSpan.setAttribute('dy', incY);
                elSpan.firstChild.data = phrase[i];
            }
        }
    }
    svg.removeChild(svg.childNodes[svg.childNodes.length - 1]);
    return elText;
}

function SVGTextLength(attrs, text) {
    var iLength = 0;
    // no text to test
    if ((typeof text != 'undefined') && (text.length > 0)) {
        var svg = getSVGnode();
        var elText = makeSVGnode('text', attrs, '', '');
        svg.appendChild(elText);
        var elSpan = makeSVGnode('tspan', {}, text);
        elText.appendChild(elSpan);
        iLength = elSpan.getComputedTextLength();
        svg.removeChild(svg.lastChild);
    }

    return iLength;
}

function getMaxSpanWidth(elText) {
    var iLength = 0;
    var iMaxSpan = 0;

    var svg = getSVGnode();
    svg.appendChild(elText);
    for (var i = 0; i < elText.childNodes.length; i++) {
        iLength = elText.childNodes[i].getComputedTextLength();
        iMaxSpan = (iLength > iMaxSpan) ? iLength : iMaxSpan;
    }
    svg.removeChild(svg.lastChild);

    return iMaxSpan;
}

function Transform(tag) {
    this.scale = 1.0;
    this.rotation = 0;
    this.xRotation = 0;
    this.yRotation = 0;
    this.xOffset = 0;
    this.yOffset = 0;

    if (arguments.length != 0) {

        var sTransform;
        if (typeof tag == 'string') sTransform = tag;
        if (typeof tag == 'object') {
            if ((tag instanceof SVGGElement) ||
                (tag instanceof SVGImageElement)) sTransform = tag.getAttribute('transform');
            if (tag[0] instanceof SVGGElement) sTransform = tag[0].getAttribute('transform');
            if ((typeof sTransform != 'undefined') && (sTransform != null) && (sTransform.length != 0)) {
                var aTransform = sTransform.split(/[,() ]/);
                var iVal;
                for (var i = 0; i < aTransform.length; i++) {
                    if (aTransform[i].toLowerCase() == 'translate') {
                        iVal = parseInt(aTransform[++i]);
                        if (!isNaN(iVal)) this.xOffset = iVal;
                        iVal = parseInt(aTransform[++i]);
                        if (!isNaN(iVal)) this.yOffset = iVal;
                        continue;
                    }
                    if (aTransform[i].toLowerCase() == 'scale') {
                        var fVal = parseFloat(aTransform[++i]);
                        if (!isNaN(fVal)) this.scale = fVal;
                        continue;
                    }
                    if (aTransform[i].toLowerCase() == 'rotate') {
                        iVal = parseInt(aTransform[++i]);
                        if (!isNaN(iVal)) this.rotation = iVal;
                        iVal = parseInt(aTransform[++i]);
                        if (!isNaN(iVal)) this.xRotation = iVal;
                        iVal = parseInt(aTransform[++i]);
                        if (!isNaN(iVal)) this.yRotation = iVal;
                    }
                }
            }
        }
    }

    this.Add = function (tag) {
        if (tag instanceof Transform) {
            this.scale *= tag.scale;
            this.xOffset += tag.xOffset;
            this.yOffset += tag.yOffset;
            this.rotation += tag.rotation;
            this.xRotation += tag.xRotation;
            this.yRotation += tag.yRotation;

        }
    };
    this.Calculate = function (oTarget) {
        try {
            var svg = getSVGnode();
            var oMatrix = oTarget.getTransformToElement(svg);  // sometimes throws errors on IE 11
            this.scale = (oMatrix.d == 1) ? 0 : oMatrix.d;
            this.xOffset = oMatrix.e;
            this.yOffset = oMatrix.f;
        }
        catch (err) {
            var oTemp = oTarget.parentNode;
            var oTransform = new Transform();
            while (oTemp.nodeName.toLowerCase() != 'svg') {
                var oNew = new Transform(oTemp);
                oTransform.Add(oNew);
                oTemp = oTemp.parentNode;
            }
            this.scale = oTransform.scale;
            this.xOffset = oTransform.xOffset;
            this.yOffset = oTransform.yOffset;
        }
    };
    this.toString = function () {
        var retVal = '';
        if (this.scale != 1.0) {
            retVal += 'scale(' + this.scale + ')';
        }
        if ((this.xOffset + this.yOffset) != 0) {
            retVal += ' translate(' + this.xOffset + ',' + this.yOffset + ')';
        }
        if ((this.rotation + this.xRotation + this.yRotation) != 0) {
            retVal += ' rotate(' + this.rotation + ',' + this.xRotation + ',' + this.yRotation + ')';
        }
        return retVal;
    };
}

function Slider(event) {

    if (arguments.length == 0) {
        $('.SliderBack')
            .off()
            .bind('click', function (event, ui) { var oSlider = new Slider(event); oSlider.Click(event, ui); });
        $('.Slider')
            .off()
            .bind('click', function (event, ui) { var oSlider = new Slider(event); oSlider.Click(event); });
        $('.SliderHandle')
            .off()
            .draggable()
            .on('drag', function (event, ui) { var oSlider = new Slider(event); oSlider.Drag(event); })
            .on('dragstop', function (event, ui) { var oSlider = new Slider(event); oSlider.Click(event); $('.SliderHandle').draggable(); });
    }

    var curLevel = 0;
    var newLevel = 0;
    var maxLevel = 0;
    var oSliderHandle;

    this.Initialise = function (event) {
        if (typeof (event.clientX) == "undefined") {
            //            $.cachenoty=generate_noty('error', '<b>event clientx is '+event.originalEvent.clientX+'</b>', 250);
            return;
        }
        var svg = event.target;
        while ((svg.tagName != 'svg') && (svg.tagName != 'body')) {
            svg = svg.parentNode;
            if (svg == null) return;
        }
        if (svg.tagName != 'svg') return;

        var oGroup = event.target.parentNode;
        var oSliderBack;
        var oSlider;
        for (var i = 0; i < oGroup.childNodes.length; i++) {
            if (oGroup.childNodes[i].id == "sliderback") {
                oSliderBack = oGroup.childNodes[i];
            }
            if (oGroup.childNodes[i].id == "slider") {
                oSlider = oGroup.childNodes[i];
            }
            if (oGroup.childNodes[i].id == "sliderhandle") {
                oSliderHandle = oGroup.childNodes[i];
            }
        }

        var pt = svg.createSVGPoint();
        pt.x = oSliderBack.getAttribute('x');
        pt.y = oSliderBack.getAttribute('y');
        var backWidth = oSliderBack.getAttribute('width');
        var matrix = oSliderBack.getScreenCTM();
        var transformed = pt.matrixTransform(matrix);
        var result = (event.clientX - transformed.x) / (backWidth * matrix.a);   // should give a percentage
        if (result < 0) result = 0;
        if (result > 1) result = 1;
        curLevel = parseInt(oSliderHandle.getAttribute("level"));
        maxLevel = parseInt(oSliderHandle.getAttribute("maxlevel"));
        newLevel = Math.round(maxLevel * result);
        var handleWidth = parseInt(oSliderHandle.getAttribute("width"));
        oSliderHandle.setAttribute("x", (backWidth * (newLevel / maxLevel)) - (handleWidth / 2));
        oSlider.setAttribute("width", (backWidth * (newLevel / maxLevel)));
    };
    this.Click = function (event) {
        this.Initialise(event);
        if (newLevel != curLevel) {
            oSliderHandle.setAttribute("level", newLevel);
            var idx = parseInt(oSliderHandle.getAttribute("devindex"));
            //            SetDimValue(idx, newLevel);  // this is synchronous and makes this quite slow
            if (window.my_config.userrights == 0) {
                HideNotify();
                ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                return;
            }
            $.ajax({
                url: "json.htm?type=command&param=switchlight&idx=" + idx + "&switchcmd=Set%20Level&level=" + newLevel,
                async: true,
                dataType: 'json'
            });
            setTimeout(function () { eval(Device.switchFunction + "();"); }, 250);
        }
    };
    this.Drag = function (event) {
        this.Initialise(event);
    };
}

function DoNothing() {
    return;
}

function decodeBase64(s) {
    var e = {}, i, b = 0, c, x, l = 0, a, r = '', w = String.fromCharCode, L = s.length;
    var A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (i = 0; i < 64; i++) { e[A.charAt(i)] = i; }
    for (x = 0; x < L; x++) {
        c = e[s.charAt(x)]; b = (b << 6) + c; l += 6;
        while (l >= 8) { ((a = (b >>> (l -= 8)) & 0xff) || (x < (L - 2))) && (r += w(a)); }
    }
    return r;
}

//  �2003 by Gavin Kistner:  http://phrogz.net/JS/classes/OOPinJS.html , http://phrogz.net/JS/classes/OOPinJS2.html
Function.prototype.inheritsFrom = function (parentClassOrObject) {
    if (parentClassOrObject.constructor == Function) {
        //Normal Inheritance
        this.prototype = new parentClassOrObject;
        this.prototype.constructor = this;
        this.prototype.parent = parentClassOrObject.prototype;
    }
    else {
        //Pure Virtual Inheritance
        this.prototype = parentClassOrObject;
        this.prototype.constructor = this;
        this.prototype.parent = parentClassOrObject;
    }
    return this;
};

function Device(item) {

    if (arguments.length != 0) {
        this.index = item.idx;
        this.floorID = (typeof item.FloorID != 'undefined') ? parseInt(item.FloorID) : 0;
        this.planID = (typeof item.PlanID != 'undefined') ? parseInt(item.PlanID) : 0;
        this.xoffset = (typeof item.XOffset != 'undefined') ? parseInt(item.XOffset) : 0;
        this.yoffset = (typeof item.YOffset != 'undefined') ? parseInt(item.YOffset) : 0;
        this.scale = (typeof item.Scale != 'undefined') ? parseFloat(item.Scale) : 1.0;
        this.width = Device.elementPadding * 50;
        this.height = Device.elementPadding * 15;
        this.type = item.Type;
        this.devSceneType = ((item.Type == 'Scene') || (item.Type == 'Group')) ? 1 : 0;
        this.subtype = item.SubType;
        this.status = (typeof item.Status == 'undefined') ? '' : item.Status;
        this.lastupdate = item.LastUpdate;
        this.protected = item.Protected;
        this.name = item.Name;
        this.imagetext = '';
        this.image = "images/unknown.png";
        this.image_opacity = 1;
        this.image2 = "";
        this.image2_opacity = 1;
        this.favorite = item.Favorite;
        this.forecastURL = (typeof item.forecast_url == 'undefined') ? '' : item.forecast_url;
        this.batteryLevel = (typeof item.BatteryLevel != 'undefined') ? item.BatteryLevel : 0;
        this.haveTimeout = (typeof item.HaveTimeout != 'undefined') ? item.HaveTimeout : false;
        this.haveCamera = ((typeof item.UsedByCamera != 'undefined') && (typeof item.CameraIdx != 'undefined')) ? item.UsedByCamera : false;
        this.cameraIdx = (typeof item.CameraIdx != 'undefined') ? item.CameraIdx : 0;
        this.haveDimmer = false;  // data from server is unreliable so inheriting classes will need to force true
        this.level = (typeof item.LevelInt != 'undefined') ? parseInt(item.LevelInt) : 0;
        this.levelMax = (typeof item.MaxDimLevel != 'undefined') ? parseInt(item.MaxDimLevel) : 0;
        if (this.level > this.levelMax) this.levelMax = this.level;
        this.data = (typeof item.Data != 'undefined') ? item.Data : '';
        this.hasNewLine = false;

        this.smallStatus = this.data;
        if (typeof item.Usage != 'undefined') {
            this.data = item.Usage;
        }
                 if (typeof item.SensorUnit != 'undefined') {
                        this.sensorunit = item.SensorUnit;
                }
        this.uniquename = item.Type.replace(/\s+/g, '') + '_' + this.floorID + '_' + this.planID + '_' + this.index;
        this.controlable = false;
        this.switchType = (typeof item.SwitchType != 'undefined') ? item.SwitchType : '';
        this.switchTypeVal = (typeof item.SwitchTypeVal != 'undefined') ? item.SwitchTypeVal : '';
        this.onClick = '';
        this.onClick2 = '';
        this.LogLink = '';
        this.EditLink = '';
        this.TimerLink = '';
        this.NotifyLink = '';
        this.WebcamLink = '';
        this.hasNotifications = (item.Notifications == "true") ? true : false;
        this.moveable = false;
        this.onFloorplan = true;
        this.showStatus = false;

        // store in page for later use
        if ($("#DeviceData")[0] != undefined) {
            var el = makeSVGnode('data', { id: this.uniquename + '_Data', item: JSON.stringify(item) }, '');
            var existing = document.getElementById(this.uniquename + '_Data');
            if (existing != null) {
                $("#DeviceData")[0].replaceChild(el, existing);
            } else {
                $("#DeviceData")[0].appendChild(el);
            }
        }

        Device.count++;
        if ((this.xoffset == 0) && (this.yoffset == 0)) {
            var iconSpacing = 75;
            var iconsPerColumn = Math.floor(Device.yImageSize / iconSpacing);
            this.yoffset += Device.notPositioned * iconSpacing;
            while ((this.yoffset + 50) > Device.yImageSize) {
                this.xoffset += iconSpacing;
                this.yoffset -= (iconsPerColumn * iconSpacing);
            }
            Device.notPositioned++;
            this.onFloorplan = false;
        }
    }

    this.setDraggable = function (draggable) {
        this.moveable = draggable;
    };
    this.drawIcon = function (parent) {
        var el;
        //Draw device values if option(s) is turned on
        if ((this.showStatus == true) && (this.smallStatus.length > 0)) {
            var nbackcolor = "#D4E1EE";
            if (this.protected == true) {
                nbackcolor = "#A4B1EE";
            }
            if (this.haveTimeout == true) {
                nbackcolor = "#DF2D3A";
            }
            if (Device.useSVGtags == true) {
                if (this.hasNewLine) {
                    var oText = makeSVGmultiline(
                        { transform: 'translate(' + Device.iconSize / 2 + ',' + (Device.iconSize + (Device.elementPadding * 1.5) + 1) + ')', 'text-anchor': 'middle', 'font-weight': 'bold', 'font-size': '80%' },
                        $.t(this.smallStatus.replace('Watt', 'W')),
                        '',
                        Device.iconSize * 3,
                        0,
                        Device.elementPadding * 2.5 + 1,
                        "<br />"
                    );
                }
                else {
                    var oText = makeSVGmultiline(
                        { transform: 'translate(' + Device.iconSize / 2 + ',' + (Device.iconSize + (Device.elementPadding * 1.5) + 1) + ')', 'text-anchor': 'middle', 'font-weight': 'bold', 'font-size': '80%' },
                        $.t(this.smallStatus.replace('Watt', 'W')),
                        '',
                        Device.iconSize * 3,
                        0,
                        Device.elementPadding * 2.5 + 1
                    );
                }

                var maxSpan = getMaxSpanWidth(oText);
                el = makeSVGnode('g', { id: this.uniquename + "_Tile", 'class': 'DeviceTile', style: (maxSpan == 0) ? 'display:none' : 'display:inline' }, '');
                var offset = (Device.iconSize / 2) - (maxSpan / 2) - Device.elementPadding;
                el.appendChild(makeSVGnode('rect', { x: offset + 2, y: Device.iconSize - (Device.elementPadding * 0.5) + 2, rx: Device.elementPadding, ry: Device.elementPadding, width: maxSpan + (Device.elementPadding * 2), height: oText.childNodes.length * Device.elementPadding * 3, 'stroke-width': '0', fill: 'black', opacity: "0.5" }, ''));
                el.appendChild(makeSVGnode('rect', { 'class': 'header', x: offset, y: Device.iconSize - (Device.elementPadding * 0.5), rx: Device.elementPadding, ry: Device.elementPadding, width: maxSpan + (Device.elementPadding * 2), height: oText.childNodes.length * Device.elementPadding * 3, style: 'fill:' + nbackcolor }, ''));
                el.appendChild(oText);
            } else {
                // html
            }

            existing = document.getElementById(this.uniquename + "_Tile");
            if (existing != undefined) {
                existing.parentNode.replaceChild(el, existing);
            } else {
                parent.appendChild(el);
            }
        }

        if (Device.useSVGtags == true) {
            el = makeSVGnode('image', {
                id: this.uniquename + "_Icon",
                'class': 'DeviceIcon',
                'xlink:href': this.image,
                width: Device.iconSize, height: Device.iconSize,
                onmouseover: (this.moveable == true) ? '' : "Device.popup('" + this.uniquename + "');",
                onmouseout: (this.moveable == true) ? '' : "Device.popupCancelDelay();",
                onclick: ((this.moveable == true)||(this.onClick=='')) ? '' : "Device.popupCancelDelay(); " + (this.controlable ? '' : '$("body").trigger("pageexit"); ') + this.onClick,
                ontouchstart: (this.moveable == true) ? '' : "Device.ignoreClick=true; Device.popup('" + this.uniquename + "');",
                ontouchend: (this.moveable == true) ? '' : "Device.popupCancelDelay();",
                style: (this.moveable == true) ? 'cursor:move;' : 'cursor:hand; -webkit-user-select: none;'
            }, '');
            el.appendChild(makeSVGnode('title', null, this.name));
        } else {
            el = makeSVGnode('img', {
                id: this.uniquename + "_Icon",
                'src': this.image,
                alt: this.name,
                width: Device.iconSize, height: Device.iconSize,
                onmouseover: (this.moveable == true) ? '' : "Device.popup('" + this.uniquename + "');",
                onclick: (this.moveable == true) ? '' : "Device.popup('" + this.uniquename + "');",
                style: (this.moveable == true) ? 'cursor:move;' : 'cursor:hand;'
            }, '');
        }

        var existing = document.getElementById(this.uniquename + "_Icon");
        if (existing != undefined) {
            existing.parentNode.replaceChild(el, existing);
        } else {
            parent.appendChild(el);
        }

        return el;
    };
    this.drawDetails = function (parent, display) {
        var nbackcolor = "#D4E1EE";
        var showme = (display == false) ? 'none' : 'inline';
        if (this.protected == true) {
            nbackcolor = "#A4B1EE";
        }
        if (this.haveTimeout == true) {
            nbackcolor = "#DF2D3A";
        }
        if (this.batteryLevel <= 10) {
            nbackcolor = "#DDDF2D";
        }
        var existing = document.getElementById(this.uniquename + "_Detail");
        var el;
        var sDirection = 'right';
        if (Device.useSVGtags == true) {
            if (existing != undefined) {
                el = existing.cloneNode(false);  // shallow clone only
                sDirection = el.getAttribute('direction');
                var oTransform = new Transform(el);
                if (sDirection == 'right') {
                    oTransform.xOffset = -Device.elementPadding;
                } else {
                    oTransform.xOffset = -(this.width - Device.iconSize - Device.elementPadding);
                }
                el.setAttribute('transform', oTransform.toString());
            } else {
                el = makeSVGnode('g', { 'class': 'DeviceDetails', id: this.uniquename + '_Detail', transform: 'translate(-' + Device.elementPadding + ',-' + Device.elementPadding * 6 + ')', width: this.width, height: this.height, direction: 'right', style: 'display:' + showme + '; -webkit-user-select: none;', onmouseleave: "$('.DeviceDetails').css('display', 'none');", 'pointer-events': 'none' }, '');
            }
            el.appendChild(makeSVGnode('rect', { id: "shadow", transform: "translate(2,2)", rx: Device.elementPadding, ry: Device.elementPadding, width: this.width, height: (el.getAttribute('expanded') != "true") ? this.height : this.height + (Device.elementPadding * 6), 'stroke-width': '0', fill: 'black', opacity: "0.3" }, ''));
            el.appendChild(makeSVGnode('rect', { 'class': 'popup', rx: Device.elementPadding, ry: Device.elementPadding, width: this.width, height: (el.getAttribute('expanded') != "true") ? this.height : this.height + (Device.elementPadding * 6), stroke: 'gray', 'stroke-width': '0.25', fill: 'url(#PopupGradient)', 'pointer-events': 'all' }, ''));
            el.appendChild(makeSVGnode('rect', {
                'class': 'header', x: Device.elementPadding, y: Device.elementPadding, rx: Device.elementPadding, ry: Device.elementPadding,
                width: (this.width - (Device.elementPadding * 2)), height: Device.elementPadding * 4, style: 'fill:' + nbackcolor
            }, ''));
            if (this.haveCamera == true) {
                el.appendChild(makeSVGnode('image', { id: "webcam", 'xlink:href': 'images/webcam.png', width: 16, height: 16, x: (this.width - (Device.elementPadding * 2) - 16), y: (Device.elementPadding * 3) - 8, onclick: this.WebcamLink, onmouseover: "cursorhand();", onmouseout: "cursordefault();", 'pointer-events': 'all' }, '', $.t('Stream Video')));
            }
            el.appendChild(makeSVGnode('text', { id: "name", x: Device.elementPadding * 2, y: Device.elementPadding * 4, 'text-anchor': 'start' }, this.name));
            el.appendChild(makeSVGnode('text', { id: "bigtext", x: (this.width - (Device.elementPadding * 2)), y: Device.elementPadding * 4, 'text-anchor': 'end', 'font-weight': 'bold' }, $.t(this.data)));

            var iOffset = ((sDirection == 'right') ? Device.elementPadding : ((this.image2 == '') ? this.width - Device.elementPadding - Device.iconSize : this.width - (Device.elementPadding * 2) - (Device.iconSize * 2)));
            var gImageGroup = makeSVGnode('g', { id: "imagegroup", transform: 'translate(' + iOffset + ',' + Device.elementPadding * 6 + ')' }, '');
            el.appendChild(gImageGroup);
            gImageGroup.appendChild(makeSVGnode('image', {
                id: "image", 'xlink:href': this.image, width: Device.iconSize, height: Device.iconSize, opacity: this.image_opacity,
                onclick: (this.onClick.length != 0) ? 'if (Device.ignoreClick!=true) {' + (this.controlable ? '' : '$("body").trigger("pageexit"); ') + this.onClick + '};' : '',
                onmouseover: (this.onClick.length != 0) ? "cursorhand()" : '',
                onmouseout: (this.onClick.length != 0) ? "cursordefault()" : '', 'pointer-events': 'all'
            }, '', $.t(this.imagetext)));
            if (this.image2 != '') {
                gImageGroup.appendChild(makeSVGnode('image', { id: "image2", x: Device.iconSize + Device.elementPadding, 'xlink:href': this.image2, width: Device.iconSize, height: Device.iconSize, opacity: this.image2_opacity, onclick: (this.onClick2.length != 0) ? this.onClick2 : '', onmouseover: (this.onClick.length != 0) ? "cursorhand()" : '', onmouseout: (this.onClick.length != 0) ? "cursordefault()" : '', 'pointer-events': 'all' }, '', $.t(this.imagetext)));
            }
            iOffset = ((sDirection == 'right') ? ((this.image2 == '') ? Device.iconSize + (Device.elementPadding * 2) : (Device.iconSize * 2 + Device.elementPadding * 3)) : Device.elementPadding * 2);
            var gStatusGroup = makeSVGnode('g', { id: "statusgroup", transform: 'translate(' + iOffset + ',' + Device.elementPadding * 6 + ')' }, '');
            el.appendChild(gStatusGroup);
            if (typeof (this.drawCustomStatus) === "function") {
                this.drawCustomStatus(gStatusGroup);
            } else if (this.haveDimmer === true) {
                gStatusGroup.appendChild(makeSVGnode('text', { id: "status", x: 0, y: Device.elementPadding * 2, 'font-weight': 'bold', 'font-size': '90%' }, TranslateStatus(this.status)));
                gStatusGroup.appendChild(makeSVGnode('rect', { id: "sliderback", 'class': "SliderBack", x: 0, y: Device.elementPadding * 3, width: Device.elementPadding * 35, height: Device.elementPadding * 2, rx: Device.elementPadding, ry: Device.elementPadding, fill: 'url(#SliderImage)', stroke: 'black', 'stroke-width': '0.5', 'pointer-events': 'all' }, '', $.t('Adjust level')));
                gStatusGroup.appendChild(makeSVGnode('rect', { id: "slider", 'class': "Slider", x: 0, y: Device.elementPadding * 3, width: (Device.elementPadding * 35) * (this.level / this.levelMax), height: Device.elementPadding * 2, rx: Device.elementPadding, ry: Device.elementPadding, fill: 'url(#SliderGradient)', stroke: 'black', 'stroke-width': '0.5', 'pointer-events': 'all' }, '', $.t('Adjust level')));
                gStatusGroup.appendChild(makeSVGnode('image', { id: "sliderhandle", 'class': "SliderHandle", x: ((Device.elementPadding * 35) * (this.level / this.levelMax)) - (Device.elementPadding * 2), y: Device.elementPadding * 2, level: this.level, maxlevel: this.levelMax, devindex: this.index, 'xlink:href': 'images/handle.png', width: Device.elementPadding * 4, height: Device.elementPadding * 4, 'pointer-events': 'all', onmouseover: "cursorhand()", onmouseout: "cursordefault()" }, '', $.t('Slide to adjust level')));
            } else {
                if (this.hasNewLine) {
                    var oStatus = makeSVGmultiline({ id: "status", transform: 'translate(0,' + Device.elementPadding * 3 + ')', 'font-weight': 'bold', 'font-size': '90%' }, TranslateStatus(this.status), '', ((this.image2 == '') ? Device.elementPadding * 37 : Device.elementPadding * 32), Device.elementPadding * -1.5, Device.elementPadding * 3, "<br />");
                }
                else {
                    var oStatus = makeSVGmultiline({ id: "status", transform: 'translate(0,' + Device.elementPadding * 3 + ')', 'font-weight': 'bold', 'font-size': '90%' }, TranslateStatus(this.status), '', ((this.image2 == '') ? Device.elementPadding * 37 : Device.elementPadding * 32), Device.elementPadding * -1.5, Device.elementPadding * 3);
                    if (oStatus.childNodes.length > 2) { // if text is too long try and squeeze it. if that doesn't work chop it at 2 lines
                        oStatus = makeSVGmultiline({ id: "status", transform: 'translate(0,' + Device.elementPadding * 3 + ')', 'font-weight': 'bold', 'font-size': '90%' }, TranslateStatus(this.status), '', ((this.image2 == '') ? Device.elementPadding * 37 : Device.elementPadding * 32), Device.elementPadding * -1.5, Device.elementPadding * 3, ' ');
                        while (oStatus.childNodes.length > 2) {
                            oStatus.removeChild(oStatus.childNodes[oStatus.childNodes.length - 1]);
                        }
                    }
                }

                gStatusGroup.appendChild(oStatus);
            }
            var gText = makeSVGnode('text', { id: "lastseen", x: 0, y: Device.elementPadding * 7.5, 'font-size': '80%' }, '');
            gStatusGroup.appendChild(gText);
            gText.appendChild(makeSVGnode('tspan', { id: "lastlabel", 'font-style': 'italic', 'font-size': '80%' }, $.t('Last Seen')));
            gText.appendChild(makeSVGnode('tspan', { id: "lastlabel", 'font-style': 'italic', 'font-size': '80%' }, ':'));
            gText.appendChild(makeSVGnode('tspan', { id: "lastupdate", 'font-size': '80%' }, ' ' + this.lastupdate));
        }
        else {  // this is not used but is included to show that the code could draw other than SVG (such as HTML5 canvas)
            el = makeSVGnode('div', { 'class': 'span4 DeviceDetails', id: this.uniquename + '_Detail', style: 'display:' + showme + '; position:relative; ' }, '');
            var table = makeSVGnode('table', { 'id': 'itemtablesmall', border: '0', cellspacing: '0', cellpadding: '0' }, '');
            el.appendChild(table);
            var tbody = makeSVGnode('tbody', {}, '');
            table.appendChild(tbody);
            var tr = makeSVGnode('tr', {}, '');
            tbody.appendChild(tr);
            tr.appendChild(makeSVGnode('td', { id: "name", style: "background-color: rgb(164,177,238);" }, this.name));
            tr.appendChild(makeSVGnode('td', { id: "bigtext" }, this.data));
            tr.appendChild(makeSVGnode('img', { id: "image", src: this.image, width: Device.iconSize, height: Device.iconSize }, ''));
            tr.appendChild(makeSVGnode('td', { id: "status" }, this.status));
            tr.appendChild(makeSVGnode('span', { id: "lastseen", 'font-size': '80%', 'font-style': 'italic' }, 'Last Seen: '));
            tr.appendChild(makeSVGnode('td', { id: "lastupdate" }, this.lastupdate));
        }

        if (existing != undefined) {
            existing.parentNode.replaceChild(el, existing);
        } else {
            parent.appendChild(el);
        }

        return el;
    }
    this.drawButtons = function (parent) {
        var bVisible = false;
        var sDirection = 'right';
        if (typeof parent != "undefined") {
            bVisible = (parent.getAttribute('expanded') == "true") ? true : false;
            sDirection = (parent.getAttribute('direction') == 'left') ? 'left' : 'right';
        }
        var el;
        var iOffset;
        if ((this.LogLink != "") || (this.TimerLink != "") || (this.NotifyLink != "") || (this.forecastURL.length != 0)) {
            if (Device.useSVGtags == true) {
                iOffset = this.width - Device.elementPadding - 16;
                if (sDirection == 'left') {
                    iOffset = (this.image2 == '') ? this.width - Device.elementPadding - Device.iconSize - 16 : this.width - (Device.elementPadding * 2) - (Device.iconSize * 2) - 16;
                }
                var twistyRotate = "";
                if (bVisible == true) {
                    twistyRotate = " rotate(180,8,8)";
                }
                parent.appendChild(makeSVGnode('image', { id: "twisty", 'xlink:href': 'images/expand16.png', width: 16, height: 16, transform: 'translate(' + iOffset + ',' + (this.height - Device.elementPadding - 16) + ')' + twistyRotate, onclick: "Device.popupExpand('" + this.uniquename + "');", onmouseover: "this.style.cursor = 'n-resize';", onmouseout: "cursordefault()", 'pointer-events': 'all' }, '', $.t('Details display')));
                el = makeSVGnode('g', { id: "detailsgroup", transform: 'translate(0,' + Device.elementPadding * 15 + ')', style: bVisible ? 'display:inline' : 'display:none' }, '');
                iOffset = ((sDirection == 'right') ? ((this.image2 == '') ? Device.iconSize + (Device.elementPadding * 2) : (Device.iconSize * 2 + Device.elementPadding * 3)) : Device.elementPadding * 2);
                gText = makeSVGnode('text', { id: "type", x: iOffset, y: Device.elementPadding, 'font-size': '80%', 'font-style': 'italic' }, '');
                el.appendChild(gText);
                gText.appendChild(makeSVGnode('tspan', { id: "typelabel", 'font-size': '80%' }, $.t('Type')));
                gText.appendChild(makeSVGnode('tspan', { id: "typelabel", 'font-size': '80%' }, ':'));
                gText.appendChild(makeSVGnode('tspan', { id: "typedetail1", 'font-size': '80%' }, ' ' + this.type));
                if (typeof this.subtype != "undefined") {
                    gText.appendChild(makeSVGnode('tspan', { id: "typedetail2", 'font-size': '80%' }, ', ' + this.subtype));
                }
                if (typeof this.switchType != "undefined") {
                    gText.appendChild(makeSVGnode('tspan', { id: "typedetail3", 'font-size': '80%' }, ', ' + this.switchType));
                }
                if (window.my_config.userrights == 2) {
                    el.appendChild(makeSVGnode('image', { id: "favorite", x: Device.elementPadding, y: Device.elementPadding * 2, 'xlink:href': (this.favorite == 1) ? 'images/favorite.png' : 'images/nofavorite.png', onclick: (this.favorite == 1) ? "Device.MakeFavorite(" + this.index + ",0);" : "Device.MakeFavorite(" + this.index + ",1);", width: '16', height: '16', onmouseover: "cursorhand()", onmouseout: "cursordefault()", 'pointer-events': 'all' }, '', $.t('Toggle dashboard display')));
                } else {
                    el.appendChild(makeSVGnode('image', { id: "favorite", x: Device.elementPadding, y: Device.elementPadding * 2, 'xlink:href': (this.favorite == 1) ? 'images/favorite.png' : 'images/nofavorite.png', width: '16', height: '16' }, '', $.t('Favorite')));
                }
                var iLength = 0;
                iOffset = Device.elementPadding * 5;
                if (this.LogLink != "") {
                    iLength = Math.floor(SVGTextLength({ 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Log'))) + (Device.elementPadding * 2);
                    el.appendChild(makeSVGnode('rect', { id: "Log", x: iOffset, y: Device.elementPadding * 2, width: iLength, height: Device.elementPadding * 3, rx: Device.elementPadding, ry: Device.elementPadding, fill: "url(#ButtonImage1)", onclick: (this.LogLink.length != 0) ? '{$("body").trigger("pageexit"); ' + this.LogLink + '}' : '', onmouseover: (this.LogLink.length != 0) ? "cursorhand()" : '', onmouseout: (this.LogLink.length != 0) ? "cursordefault()" : '', 'pointer-events': 'all' }, $.t('Log'), ''));
                    el.appendChild(makeSVGnode('text', { x: iOffset + (iLength / 2), y: Device.elementPadding * 4, 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Log'), ''));
                    iOffset += iLength + Device.elementPadding;
                }
                if ((window.my_config.userrights == 2) && (this.EditLink != "")) {
                    iLength = Math.floor(SVGTextLength({ 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Edit'))) + (Device.elementPadding * 2);
                    el.appendChild(makeSVGnode('rect', { id: "Edit", x: iOffset, y: Device.elementPadding * 2, width: iLength, height: Device.elementPadding * 3, rx: Device.elementPadding, ry: Device.elementPadding, fill: "url(#ButtonImage1)", onclick: (this.EditLink.length != 0) ? '{$("body").trigger("pageexit"); ' + this.EditLink + '}' : '', onmouseover: (this.EditLink.length != 0) ? "cursorhand()" : '', onmouseout: (this.EditLink.length != 0) ? "cursordefault()" : '', 'pointer-events': 'all' }, $.t('Edit'), ''));
                    el.appendChild(makeSVGnode('text', { x: iOffset + (iLength / 2), y: Device.elementPadding * 4, 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Edit'), ''));
                    iOffset += iLength + Device.elementPadding;
                }
                if (this.TimerLink != "") {
                    iLength = Math.floor(SVGTextLength({ 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Timers'))) + (Device.elementPadding * 2);
                    el.appendChild(makeSVGnode('rect', { id: "Timers", x: iOffset, y: Device.elementPadding * 2, width: iLength, height: Device.elementPadding * 3, rx: Device.elementPadding, ry: Device.elementPadding, fill: "url(#ButtonImage1)", 'ng-controller': 'ScenesController', onclick: (this.TimerLink.length != 0) ? '{$("body").trigger("pageexit"); ' + this.TimerLink + '}' : '', onmouseover: (this.TimerLink.length != 0) ? "cursorhand()" : '', onmouseout: (this.TimerLink.length != 0) ? "cursordefault()" : '', 'pointer-events': 'all' }, $.t('Timers'), ''));
                    el.appendChild(makeSVGnode('text', { x: iOffset + (iLength / 2), y: Device.elementPadding * 4, 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Timers'), ''));
                    iOffset += iLength + Device.elementPadding;
                }
                if ((window.my_config.userrights == 2) && (this.NotifyLink != "")) {
                    iLength = Math.floor(SVGTextLength({ 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Notifications'))) + (Device.elementPadding * 2);
                    el.appendChild(makeSVGnode('rect', { id: "Notifications", x: iOffset, y: Device.elementPadding * 2, width: iLength, height: Device.elementPadding * 3, rx: Device.elementPadding, ry: Device.elementPadding, fill: (this.hasNotifications == true) ? "url(#ButtonImage2)" : "url(#ButtonImage1)", onclick: (this.NotifyLink.length != 0) ? '{$("body").trigger("pageexit"); ' + this.NotifyLink + '}' : '', onmouseover: (this.NotifyLink.length != 0) ? "cursorhand()" : '', onmouseout: (this.NotifyLink.length != 0) ? "cursordefault()" : '', 'pointer-events': 'all' }, $.t('Notifications'), ''));
                    el.appendChild(makeSVGnode('text', { x: iOffset + (iLength / 2), y: Device.elementPadding * 4, 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Notifications'), ''));
                    iOffset += iLength + Device.elementPadding;
                }
                if (this.forecastURL.length != 0) {
                    iLength = Math.floor(SVGTextLength({ 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Forecast'))) + (Device.elementPadding * 2);
                    el.appendChild(makeSVGnode('rect', { id: "Forecast", x: iOffset, y: Device.elementPadding * 2, width: iLength, height: Device.elementPadding * 3, rx: Device.elementPadding, ry: Device.elementPadding, fill: "url(#ButtonImage1)", onclick: "window.open(decodeBase64('" + this.forecastURL + "'));", onmouseover: "cursorhand()", onmouseout: "cursordefault()", 'pointer-events': 'all' }, $.t('Forecast'), ''));
                    el.appendChild(makeSVGnode('text', { x: iOffset + (iLength / 2), y: Device.elementPadding * 4, 'text-anchor': 'middle', 'font-size': '75%', fill: 'white' }, $.t('Forecast'), ''));
                    iOffset += iLength + Device.elementPadding;
                }
            } else {
                // insert HTML here
            }

            if (typeof parent != "undefined") {
                parent.appendChild(el);
            }
        }

        return el;
    }
    this.htmlMinimum = function (parent) {
        var mainEl;
        var mainEl = document.getElementById(this.uniquename);
        if (mainEl == undefined) {
            if (this.moveable == true) {
                mainEl = makeSVGnode('g', { id: this.uniquename, 'class': 'Device_' + this.index, transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')', idx: this.index, xoffset: this.xoffset, yoffset: this.yoffset, devscenetype: this.devSceneType, onfloorplan: this.onFloorplan, opacity: (this.onFloorplan == true) ? '' : '0.5' }, '');
            } else {
                mainEl = makeSVGnode('g', { id: this.uniquename, 'class': 'Device_' + this.index, transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')' }, '');
            }
            this.drawIcon(mainEl);
            ($("#DeviceIcons")[0] == undefined) ? parent.appendChild(mainEl) : $("#DeviceIcons")[0].appendChild(mainEl);

            if (this.moveable == true) {
                mainEl = makeSVGnode('g', { id: this.uniquename, transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')', idx: this.index, xoffset: this.xoffset, yoffset: this.yoffset }, '');
            } else {
                mainEl = makeSVGnode('g', { id: this.uniquename, transform: 'translate(' + this.xoffset + ',' + this.yoffset + ') scale(' + this.scale + ')' }, '');
            }
            var oDetail = this.drawDetails(mainEl, false);
            ($("#DeviceDetails")[0] == undefined) ? parent.appendChild(mainEl) : $("#DeviceDetails")[0].appendChild(mainEl);
            this.drawButtons(oDetail);
        }
        else {
            this.drawIcon(mainEl);
            var oDetail = this.drawDetails(mainEl, false);
            this.drawButtons(oDetail);
        }

        if (this.haveDimmer == true) {
            var oSlider = new Slider();
        }

        return mainEl;
    }
    this.htmlMedium = function () {
        return this.drawDetails();
    }
    this.htmlMaximum = function () {
        return this.drawDetails() + this.drawButtons();
    }
    this.htmlMobile = function () {
        return this.drawDetails();
    }
}
Device.count = 0;
Device.notPositioned = 0;
Device.useSVGtags = false;
Device.backFunction = 'DoNothing';
Device.switchFunction = 'DoNothing';
Device.contentTag = '';
Device.xImageSize = 1280;
Device.yImageSize = 720;
Device.iconSize = 32;
Device.elementPadding = 5;
Device.showSensorValues = true;
Device.showSceneNames = false;
Device.showSwitchValues = false;
Device.checkDefs = function () {
    if (Device.useSVGtags == true) {
        var divFloors = $(".imageparent");
        if (divFloors.length == 0) return;
        var cont = divFloors[0].children[0];
        if ($("#DeviceData")[0] == undefined) {
            cont.appendChild(makeSVGnode('g', { id: 'DeviceData' }, ''));
        }
        if ($("#DeviceDefs")[0] == undefined) {
            if (cont != undefined) {
                var defs = makeSVGnode('defs', { id: 'DeviceDefs' }, '');
                cont.appendChild(defs);
                var linGrad = makeSVGnode('linearGradient', { id: 'PopupGradient', x1: "0%", y1: "0%", x2: "100%", y2: "100%" }, '');
                defs.appendChild(linGrad);
                linGrad.appendChild(makeSVGnode('stop', { offset: "25%", style: "stop-color:rgb(255,255,255);stop-opacity:1" }, ''));
                linGrad.appendChild(makeSVGnode('stop', { offset: "100%", style: "stop-color:rgb(225,225,225);stop-opacity:1" }, ''));
                linGrad = makeSVGnode('linearGradient', { id: 'SliderGradient', x1: "0%", y1: "0%", x2: "0%", y2: "100%" }, '');
                defs.appendChild(linGrad);
                linGrad.appendChild(makeSVGnode('stop', { offset: "0%", style: "stop-color:#11AAFD;stop-opacity:1" }, ''));
                linGrad.appendChild(makeSVGnode('stop', { offset: "100%", style: "stop-color:#0199E0;stop-opacity:1" }, ''));
                var imgPattern = makeSVGnode('pattern', { id: 'SliderImage', width: Device.elementPadding * 2, height: Device.elementPadding * 2, patternUnits: "userSpaceOnUse" }, '');
                defs.appendChild(imgPattern);
                imgPattern.appendChild(makeSVGnode('image', { 'xlink:href': "css/images/bg-track.png", width: Device.elementPadding * 2, height: Device.elementPadding * 2 }, ''));
                imgPattern = makeSVGnode('pattern', { id: 'ButtonImage1', width: Device.elementPadding * 2, height: Device.elementPadding * 2, patternUnits: "userSpaceOnUse" }, '');
                defs.appendChild(imgPattern);
                imgPattern.appendChild(makeSVGnode('image', { 'xlink:href': "css/images/img03b.jpg", transform: 'rotate(180,8,17)', width: 16, height: 37 }, ''));
                imgPattern = makeSVGnode('pattern', { id: 'ButtonImage2', width: Device.elementPadding * 2, height: Device.elementPadding * 2, patternUnits: "userSpaceOnUse" }, '');
                defs.appendChild(imgPattern);
                imgPattern.appendChild(makeSVGnode('image', { 'xlink:href': "css/images/img03bs.jpg", width: 16, height: 37 }, ''));
            }
        }
    }
}
Device.initialise = function () {
    Device.count = 0;
    Device.notPositioned = 0;
    if (Device.useSVGtags == true) {
        // clean up existing data if it exists
        $("#DeviceData").empty();
        $("#DeviceDetails").empty();
        $("#DeviceIcons").empty();

        var cont = getSVGnode();
        if (cont != undefined) {
            if ($("#DeviceContainer")[0] == undefined) {
                var devCont = makeSVGnode('g', { id: 'DeviceContainer' }, '');
                cont.appendChild(devCont);
                Device.checkDefs();
                if ($("#DeviceIcons")[0] == undefined) {
                        devCont.appendChild(makeSVGnode('g', { id: 'DeviceIcons' }, ''));
                }
                if ($("#DeviceDetails")[0] == undefined) {
                        devCont.appendChild(makeSVGnode('g', { id: 'DeviceDetails' }, ''));
                }
                if ($("#DeviceData")[0] == undefined) {
                        devCont.appendChild(makeSVGnode('g', { id: 'DeviceData' }, ''));
                }
            }
        }
    }
}
Device.create = function (item) {
    var dev;
    var type = '';

    // if we got a string instead of an object then convert it
    if (typeof item === 'string') {
        item = JSON.parse(item);
    }
    // Anomalies in device pattern (Scenes & Dusk sensors say they are  lights(???)
    if (item.Type === 'Scene') {
        type = 'scene';
    } else if (item.Type === 'Group') {
        type = 'group';
    } else if ((item.Type === 'General') && (item.SubType === 'Barometer')) {
        type = 'baro';
    } else if ((item.Type === 'General') && (item.SubType === 'Custom Sensor')) {
        type = 'custom';
    } else if ((item.Type === 'Thermostat') && (item.SubType === 'SetPoint')) {
        type = 'setpoint';  // Instead of the TypeImg (which changes when using custom images)
    } else if (
        (item.SwitchType === 'Dusk Sensor') ||
        (item.SwitchType === 'Selector')
    ) {
        type = item.SwitchType.toLowerCase()
    } else {
        type = item.TypeImg.toLowerCase();
        if ((item.CustomImage !== 0) && (typeof item.Image !== 'undefined')) {
            type = item.Image.toLowerCase();
        }
    }
    switch (type) {
        case "alert":
            dev = new Alert(item);
            break;
        case "baro":
            dev = new Baro(item);
            break;
        case "blinds":
        case "blinds inverted":
        case "blinds percentage":
            dev = new Blinds(item);
            break;
        case "contact":
            dev = new Contact(item);
            break;
        case "counter":
            dev = new Counter(item);
            break;
        case "current":
            dev = new Current(item);
            break;
        case "custom":
            dev = new Custom(item);
            break;
        case "dimmer":
            dev = new Dimmer(item);
            break;
        case "door":
            dev = new Door(item);
            break;
        case "door contact":
            dev = new DoorContact(item);
            break;
        case "doorbell":
            dev = new Doorbell(item);
            break;
        case "dusk sensor":
            dev = new DuskSensor(item);
            break;
        case "fan":
            dev = new BinarySwitch(item);
            break;
        case "humidity":
            dev = new Humidity(item);
            break;
        case "lightbulb":
            dev = new Lightbulb(item);
            break;
        case "lux":
            dev = new VariableSensor(item);
            break;
        case "motion":
            dev = new Motion(item);
            break;
        case "media":
            dev = new MediaPlayer(item);
            break;
        case "group":
            dev = new Group(item);
            break;
        case "hardware":
            dev = new Hardware(item);
            break;
        case "push":
            dev = new Pushon(item);
            break;
        case "pushoff":
            dev = new Pushoff(item);
            break;
        case "override":
        case "override_mini":
        case "setpoint":
            dev = new SetPoint(item);
            break;
        case "radiation":
            dev = new Radiation(item);
            break;
        case "rain":
            dev = new Rain(item);
            break;
        case "scene":
            dev = new Scene(item);
            break;
        case "siren":
            dev = new Siren(item);
            break;
        case "smoke":
            dev = new Smoke(item);
            break;
        case "speaker":
            dev = new Sound(item);
            break;
        case "temp":
        case "temperature":
            dev = new Temperature(item);
            break;
        case "text":
            dev = new Text(item);
            break;
        case "visibility":
            dev = new Visibility(item);
            break;
        case "venetian blinds":
            dev = new Blinds(item);
            break;
        case "wind":
            dev = new Wind(item);
            break;
        case "selector":
            dev = new Selector(item);
            break;
        default:
            // unknown image type (or user custom image)  so use switch type to determine what to show
            if (typeof item.SwitchType != 'undefined') {
                switch (item.SwitchType.toLowerCase()) {
                    case "media player":
                        dev = new MediaPlayer(item);
                        break;
                    default:
                        dev = new Switch(item);
                        break;
                }
            } else {
                dev = new VariableSensor(item);
            }
    }

    return dev;
}
Device.scale = function (attr) {
    if (typeof $("#DeviceContainer")[0] != 'undefined') {
        $("#DeviceContainer")[0].setAttribute("transform", attr);
    }
    return;
}
Device.ignoreClick = false;
Device.popupDelay = 0;
Device.popupDelayDevice = "";
Device.popupDelayTimer = 0;
Device.popupCancelDelay = function () {
    if (Device.popupDelayTimer != 0) {
        clearTimeout(Device.popupDelayTimer);
    }
    Device.popupDelayDevice = "";
    Device.popupDelayTimer = 0;
}
Device.popup = function (target) {
    var ignorePopupDelay = (Device.popupDelayDevice != "");
    Device.popupCancelDelay();
    $('.DeviceDetails').css('display', 'none');   // hide all popups
    if (Device.expandVar != null) {
        clearInterval(Device.expandVar);
        Device.expandVar = null;
    }
    Device.expandRect = null;
    Device.expandShadow = null;

    // Defer showing popup if a delay has been specified
    if ((ignorePopupDelay == false) && (Device.popupDelay >= 0)) {
        Device.popupDelayDevice = target;
        Device.popupDelayTimer = setTimeout(function () { Device.popup(Device.popupDelayDevice); }, Device.popupDelay);
        return;
    }

    // try to check if the popup needs to be flipped, if this can't be done just display it
    var oTarget = document.getElementById(target + "_Detail");
    var oTemp = oTarget.parentNode;
    var oTransform = new Transform();
    while (oTemp.nodeName.toLowerCase() != 'svg') {
        var oNew = new Transform(oTemp);
        oTransform.Add(oNew);
        oTemp = oTemp.parentNode;
    }
    oTarget.setAttribute('style', 'display:inline; -webkit-user-select: none;');

    var imageWidth = (oTransform.scale == 0) ? Device.xImageSize : Device.xImageSize / oTransform.scale;
    var requiredWidth = oTransform.xOffset + parseInt(oTarget.getAttribute('width'));
    var sDir = (imageWidth < requiredWidth) ? 'left' : 'right';
    if (sDir != oTarget.getAttribute('direction')) {
        oTarget.setAttribute('direction', sDir);
    }
    oTarget.setAttribute("expanded", "false");
    Device.popupRedraw(target);
}
Device.popupRedraw = function (target) {
    var oData = document.getElementById(target + "_Data");
    if ((typeof oData != "undefined") && (oData != null)) {
        var sData = oData.getAttribute('item');
        var oDevice = new Device.create(sData);
        var oTarget = document.getElementById(target + "_Detail");
        if (typeof oTarget != "undefined") {
            Device.checkDefs();
            oDevice.htmlMinimum(oTarget.parentNode);
        }
    }
    return;
}
Device.expandTarget = null;
Device.expandRect = null;
Device.expandShadow = null;
Device.expandVar = null;
Device.expandRot = null;
Device.expandInt = null;
Device.expandInc = null;
Device.expandEnd = null;
Device.popupExpand = function (target) {
    var oTarget = document.getElementById(target + "_Detail");
    if ((typeof oTarget != "undefined") && (oTarget != null)) {
        Device.expandTarget = target;
        var oDetails;
        for (var i = 0; i < oTarget.childNodes.length; i++) {
            if (oTarget.childNodes[i].className.baseVal == "popup") {
                Device.expandRect = oTarget.childNodes[i];
            }
            if (oTarget.childNodes[i].id == "shadow") {
                Device.expandShadow = oTarget.childNodes[i];
            }
            if (oTarget.childNodes[i].id == "twisty") {
                Device.expandRot = oTarget.childNodes[i];
                var oTransform = new Transform(Device.expandRot);
                if (oTransform.rotation == 0) {
                    oTransform.rotation = 1;
                }
                oTransform.xRotation = 8;
                oTransform.yRotation = 8;
                Device.expandRot.setAttribute("transform", oTransform.toString());
            }
            if (oTarget.childNodes[i].id == "detailsgroup") {
                oDetails = oTarget.childNodes[i];
            }
        }
        if (oTarget.getAttribute("expanded") == "true") {
            Device.expandInc = -1;
            Device.expandEnd = 75;
            if (typeof oDetails != "undefined") {
                oDetails.setAttribute("style", "display:none");
            }
            oTarget.setAttribute("expanded", "false");
        } else {
            Device.expandInc = 1;
            Device.expandEnd = 105;
            oTarget.setAttribute("expanded", "true");
        }
        Device.expandInt = Math.round(400 / (105 - 75));
        Device.expandVar = setInterval(function () { Device.popupExpansion() }, Device.expandInt);
    }
}
Device.popupExpansion = function () {
    if ((Device.expandRect == null) || (Device.expandShadow == null)) {  // this should never happen so just stop everything if it does !
        if (Device.expandVar != null) {
            clearInterval(Device.expandVar);
            Device.expandVar = null;
        }
        return;
    }
    var newHeight = parseInt(Device.expandRect.getAttribute("height")) + Device.expandInc;
    Device.expandRect.setAttribute("height", newHeight);
    Device.expandShadow.setAttribute("height", newHeight + 2);
    var oTransform = new Transform(Device.expandRot);
    oTransform.rotation += Math.round(180 / (105 - 75)) * Device.expandInc;
    Device.expandRot.setAttribute("transform", oTransform.toString());
    if (((Device.expandInc < 0) && (newHeight <= Device.expandEnd)) ||
        ((Device.expandInc > 0) && (newHeight >= Device.expandEnd))) {
        clearInterval(Device.expandVar);
        Device.expandRect.setAttribute("height", Device.expandEnd);
        Device.expandShadow.setAttribute("height", Device.expandEnd + 2);
        Device.expandRect = null;
        Device.expandShadow = null;
        Device.expandVar = null;
        Device.popupRedraw(Device.expandTarget);
    }
}
Device.MakeFavorite = function (id, isfavorite) {
    if (window.my_config.userrights != 2) {
        HideNotify();
        ShowNotify($.t('You do not have permission to do that!'), 2500, true);
        return;
    }
    clearInterval($.myglobals.refreshTimer);
    $.ajax({
        url: "json.htm?type=command&param=makefavorite&idx=" + id + "&isfavorite=" + isfavorite,
        async: false,
        dataType: 'json',
        success: function (data) {
            window.myglobals.LastUpdate = 0;
            eval(Device.switchFunction + "();");
        }
    });
}

function Sensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = "images/" + item.TypeImg + "48.png";

        var sensorType = this.type.replace(/\s/g, '');

        if (sensorType === 'General') {
            this.LogLink = "window.location.href = '#/Devices/" + this.index + "/Log'";
        } else {
            this.LogLink = this.onClick = "Show" + sensorType + "Log('#" + Device.contentTag + "','" + Device.backFunction + "','" + this.index + "','" + this.name + "', '" + this.switchTypeVal + "');";
        }

        this.imagetext = "Show graph";
        this.NotifyLink = "window.location.href = '#/Devices/" + this.index + "/Notifications'";

        if (this.haveCamera == true) this.WebcamLink = "javascript:ShowCameraLiveStream('" + this.name + "','" + this.cameraIdx + "')";
        this.showStatus = (Device.showSensorValues == true);
    }
}
Sensor.inheritsFrom(Device);

function BinarySensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
    }
}
BinarySensor.inheritsFrom(Sensor);

function VariableSensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
    }
}
VariableSensor.inheritsFrom(Sensor);

function SecuritySensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = (this.status == "On") ? "images/" + item.TypeImg + "48-on.png" : "images/" + item.TypeImg + "48-off.png";
    }
}
SecuritySensor.inheritsFrom(BinarySensor);

function TemperatureSensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = "images/temp48.png";
        this.status = $.t('Temp') + ': ' + this.data;
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
    }
}
TemperatureSensor.inheritsFrom(VariableSensor);

function UtilitySensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        //        this.EditLink = "EditUtilityDevice(" + this.index + ",'" + this.name + "');";
    }
}
UtilitySensor.inheritsFrom(VariableSensor);

function WeatherSensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
    }
}
WeatherSensor.inheritsFrom(VariableSensor);

function Switch(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);

        var bIsOff = (
			(this.status == "Off")
			|| (item.Status == 'Closed')
			|| (item.Status == 'Locked')
        );
        
        if (item.CustomImage != 0) {
            this.image = (bIsOff) ? "images/" + item.Image + "48_Off.png" : "images/" + item.Image + "48_On.png";
        } else {
            this.image = (bIsOff) ? "images/" + item.TypeImg + "48_Off.png" : "images/" + item.TypeImg + "48_On.png";
        }
        this.data = '';
        this.LogLink = "window.location.href = '#/Devices/" + this.index + "/Log'";
		this.showStatus = (Device.showSwitchValues == true);
		this.imagetext = "Activate switch";
		this.controlable = true;
		this.onClick = "SwitchLight(" + this.index + ",'" + ((this.status == "Off") ? "On" : "Off") + "'," + this.protected + ");";
    }
}
Switch.inheritsFrom(Sensor);

function BinarySwitch(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.onClick = "SwitchLight(" + this.index + ",'" + ((this.status == "Off") ? "On" : "Off") + "'," + this.protected + ");";
    }
}
BinarySwitch.inheritsFrom(Switch);


function Alert(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
        this.NotifyLink = "";	
		this.data = item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
		if (this.data.indexOf("<br />") != -1) {
			this.hasNewLine = true;
		}		
        this.status = this.data;
        this.data = "";
        if (typeof item.Level != 'undefined') {
            this.image = "images/Alert48_" + item.Level + ".png";
        }
    }
}
Alert.inheritsFrom(Sensor);

function Baro(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (this.name == 'Baro') this.name = 'Barometer';
        this.image = "images/baro48.png";
        this.LogLink = this.onClick = "ShowBaroLog('#" + Device.contentTag + "','" + Device.backFunction + "','" + this.index + "','" + this.name + "');";
        if (typeof item.Barometer != 'undefined') {
            this.data = this.smallStatus = item.Barometer + ' hPa';
            if (typeof item.ForecastStr != 'undefined') {
                this.status = item.Barometer + ' hPa, ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);
            }
            else {
                this.status = item.Barometer + ' hPa';
            }
            if (typeof item.Altitude != 'undefined') {
                this.status += ', Altitude: ' + item.Altitude + ' meter';
            }
        }
    }
}
Baro.inheritsFrom(WeatherSensor);

function Blinds(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.data = '';
        if (item.Status == 'Closed') {
            this.image = 'images/blinds48sel.png';
            this.image2 = 'images/blindsopen48.png';
        }
        else {
            this.image = 'images/blindsopen48sel.png';
            this.image2 = 'images/blinds48.png';
        }
		this.onClick = 'SwitchLight(' + this.index + ",'" + ((item.SwitchType == "Blinds Inverted") ? 'On' : 'Off') + "'," + this.protected + ');';
		this.onClick2 = 'SwitchLight(' + this.index + ",'" + ((item.SwitchType == "Blinds Inverted") ? 'Off' : 'On') + "'," + this.protected + ');';
        if (item.SwitchType == "Blinds Percentage") {
            this.haveDimmer = true;
            this.image2 = '';
            this.onClick2 = '';
        }
    }
}
Blinds.inheritsFrom(Switch);

function Counter(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if(item.Image == undefined)
            this.image = "images/counter.png";
        else
            this.image = "images/"+item.Image+".png";
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";

        if (typeof item.CounterToday != 'undefined') {
            this.status += ' ' + $.t("Today") + ': ' + item.CounterToday;
            this.smallStatus = item.CounterToday;
        }
        if (typeof item.CounterDeliv != 'undefined') {
            if (item.CounterDeliv != 0) {
                if (item.UsageDeliv.charAt(0) != 0) {
                    this.status += '-' + item.UsageDeliv;
                }
                this.status += ', ' + $.t("Return") + ': ' + item.CounterDelivToday;
            }
        }
    }
}
Counter.inheritsFrom(UtilitySensor);

function Contact(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
		this.image = (this.status == "Closed") ? "images/" + item.Image + "48_Off.png" : "images/" + item.Image + "48_On.png";
        this.data = '';
        this.smallStatus = this.status;
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
    }
}
Contact.inheritsFrom(BinarySensor);

function Current(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.status = '';
        if (typeof item.Usage != 'undefined') {
            this.status = (item.Usage != this.data) ? item.Usage : '';
        }
        if (typeof item.CounterToday != 'undefined') {
            this.status += ' ' + $.t("Today") + ': ' + item.CounterToday;
        }
        if (typeof item.CounterDeliv != 'undefined') {
            if (item.CounterDeliv != 0) {
                if (item.UsageDeliv.charAt(0) != 0) {
                    this.status += '-' + item.UsageDeliv;
                }
                this.status += ', ' + $.t("Return") + ': ' + item.CounterDelivToday;
            }
        }
        switch (this.type) {
            case "Energy":
                this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
                this.smallStatus = this.data;
                break;
            case "Usage":
                this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
                break;
            case "General":
                switch (this.subtype) {
                    case "kWh":
                        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
                        this.smallStatus = this.data;
                        break;
                    case "Voltage":
                        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
                        this.smallStatus = this.data;
                        break;
                    default:
                        this.LogLink = this.onClick = "ShowCurrentLog('#" + Device.contentTag + "','" + Device.backFunction + "','" + this.index + "','" + this.name + "', '" + this.switchTypeVal + "');";
                        break;
                }
                break;
            default:
                this.LogLink = this.onClick = "ShowCurrentLog('#" + Device.contentTag + "','" + Device.backFunction + "','" + this.index + "','" + this.name + "', '" + this.switchTypeVal + "');";
                break;
        }
        this.smallStatus = this.smallStatus.split(', ')[0];
    }
}
Current.inheritsFrom(UtilitySensor);

function Custom(item) {
	if (arguments.length != 0) {
		this.parent.constructor(item);
		if (item.CustomImage != 0) {
			this.image = "images/" + item.Image + "48_On.png";
		} else {
			this.image = "images/Custom.png";
		}
		this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
		this.data = '';
	}
}
Custom.inheritsFrom(UtilitySensor);

function Dimmer(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.haveDimmer = true;
        this.data = '';
        this.smallStatus = (this.status == "Off") ? 'Off' : item.Level + "%";
        if (item.CustomImage != 0) {
            this.image = (this.status == "Off") ? "images/" + item.Image + "48_Off.png" : "images/" + item.Image + "48_On.png";
        } else {
			this.image = (this.status == "Off") ? "images/Dimmer48_Off.png" : "images/Dimmer48_On.png";
        }
        this.status = TranslateStatus(this.status);
    }
}
Dimmer.inheritsFrom(Switch);

function Door(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (item.CustomImage == 0) {
            this.image = ((this.status == "Locked")||(this.status == "Closed")) ? "images/" + item.Image + "48_Off.png" : this.image = "images/" + item.Image + "48_On.png";
        }
        this.data = '';
        this.NotifyLink = this.onClick = "";
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
    }
}
Door.inheritsFrom(BinarySwitch);

function DoorContact(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (item.CustomImage == 0) {
            this.image = (this.status == "Closed") ? "images/" + item.Image + "48_Off.png" : this.image = "images/" + item.Image + "48_On.png";
        }
        this.imagetext = "";
        this.NotifyLink = this.onClick = "";
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
        this.data = '';
    }
}
DoorContact.inheritsFrom(BinarySensor);

function Doorbell(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = "images/doorbell48.png";
        this.data = '';
    }
}
Doorbell.inheritsFrom(BinarySwitch);

function DuskSensor(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = (item.Status == 'On') ? "images/uvdark.png" : this.image = "images/uvsunny.png";
        this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
        this.data = '';
    }
}
DuskSensor.inheritsFrom(BinarySensor);

function Group(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = 'images/Push48_On.png';
        this.onClick = 'SwitchScene(' + this.index + ", 'On', undefined, " + this.protected + ');';
        this.image2 = 'images/Push48_Off.png';
        this.onClick2 = 'SwitchScene(' + this.index + ", 'Off', undefined, " + this.protected + ');';
        (this.status == 'Off') ? this.image2_opacity = 0.5 : this.image_opacity = 0.5;
        this.data = '';
        this.showStatus = (Device.showSceneNames || Device.showSwitchValues);
        this.smallStatus = (Device.showSwitchValues == true) ? this.status : this.name;
    }
}
Group.inheritsFrom(Switch);

function Hardware(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);

        if (this.subtype === 'General' || this.subtype === 'Percentage') {
            this.LogLink = "window.location.href = '#/Devices/" + this.index + "/Log'";
        } else {
            this.LogLink = this.onClick = "Show" + this.subtype + "Log('#" + Device.contentTag + "','" + Device.backFunction + "','" + this.index + "','" + this.name + "', '" + this.switchTypeVal + "');";
        }

        if (item.CustomImage == 0) {
            switch (item.SubType.toLowerCase()) {
                case "percentage":
                    this.image = "images/Percentage48.png";
                    break;
            }
        } else {
            this.image = "images/"+ item.Image + "48_On.png";
        }
    }
}
Hardware.inheritsFrom(UtilitySensor);

function Humidity(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = "images/moisture48.png";
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
        if (typeof item.Humidity != 'undefined') {
            this.data = this.smallStatus = item.Humidity + '%';
            this.status = $.t('Humidity') + ': ' + item.Humidity + '%';
            if (typeof item.HumidityStatus != 'undefined') {
                this.status += ', ' + $.t(item.HumidityStatus);
            }
            if (typeof item.DewPoint != 'undefined') {
                this.status += ', ' + $.t("Dew Point") + ": " + item.DewPoint + '\u00B0' + $.myglobals.tempsign;
            }
        }
    }
}
Humidity.inheritsFrom(WeatherSensor);

function Lightbulb(item) {
    if (arguments.length != 0) {
        item.TypeImg = "Light";
        this.parent.constructor(item);
    }
}
Lightbulb.inheritsFrom(BinarySwitch);

function MediaPlayer(item) {
    if (arguments.length != 0) {
        //        item.TypeImg = "Media";
        if (item.Status.length == 1) item.Status = 'Off';
        this.parent.constructor(item);
        this.image2 = 'images/remote48.png';
        this.onClick2 = 'ShowMediaRemote(\'' + this.name + '\',' + this.index + ');';
        if (this.status == 'Off') this.image2_opacity = 0.5;
        this.imagetext = "";
        this.data = this.status;
        this.status = item.Data.replace(';', ', ');
        while (this.status.search(';') != -1) this.status = this.status.replace(';', ', ');
    }
}
MediaPlayer.inheritsFrom(BinarySwitch);

function Motion(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (item.CustomImage == 0) {
            this.image = (this.status == "On") ? "images/" + item.TypeImg + "48-on.png" : "images/" + item.TypeImg + "48-off.png";
        }
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
        this.data = '';
        this.smallStatus = this.status;
    }
}
Motion.inheritsFrom(SecuritySensor);

function Pushon(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
		this.image = "images/" + item.Image + "48_On.png";
        this.onClick = "SwitchLight(" + this.index + ",'On'," + this.protected + ");";
    }
}
Pushon.inheritsFrom(BinarySwitch);

function Pushoff(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = "images/" + item.Image + "48_Off.png";
        this.onClick = "SwitchLight(" + this.index + ",'Off'," + this.protected + ");";
    }
}
Pushoff.inheritsFrom(BinarySwitch);

function Radiation(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
    }
}
Radiation.inheritsFrom(WeatherSensor);

function Rain(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (typeof item.Rain != 'undefined') {
            this.status = item.Rain;
            if ($.isNumeric(item.Rain)) this.status += ' mm';
            this.data = this.smallStatus = this.status;
            if (typeof item.RainRate != 'undefined') {
                if (item.RainRate != 0) {
                    this.status += ', Rate: ' + item.RainRate + ' mm/h';
                }
            }
        }
    }
}
Rain.inheritsFrom(WeatherSensor);

function Scene(item) {
    if (arguments.length != 0) {
        item.Status = 'Off';
        this.parent.constructor(item);
        this.onClick = 'SwitchScene(' + this.index + ", 'On','" + Device.backFunction + "', " + this.protected + ');';
        this.data = '';
        this.LogLink = '';
        this.showStatus = Device.showSceneNames;
        this.smallStatus = this.name;
    }
}
Scene.inheritsFrom(Pushon);

function SetPoint(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (item.CustomImage != 0 && typeof item.Image != 'undefined') {
            this.image = "images/" + item.Image + ".png";
        } else {
            this.image = "images/override.png";
        }
        if ($.isNumeric(this.data)) this.data += '\u00B0' + $.myglobals.tempsign;
        if ($.isNumeric(this.status)) this.status += '\u00B0' + $.myglobals.tempsign;
        var pattern = new RegExp('\\d\\s' + $.myglobals.tempsign + '\\b');
        while (this.data.search(pattern) > 0) this.data = this.data.setCharAt(this.data.search(pattern) + 1, '\u00B0');
        while (this.status.search(pattern) > 0) this.status = this.status.setCharAt(this.status.search(pattern) + 1, '\u00B0');
        this.imagetext = "Set Temp";
        this.controlable = true;
        this.onClick = 'ShowSetpointPopup(' + event + ', ' + this.index + ', ' + this.protected + ' , "' + this.data + '");';
    }
}
SetPoint.inheritsFrom(TemperatureSensor);

function Siren(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = ((item.Status == 'On') || (item.Status == 'Chime') || (item.Status == 'Group On') || (item.Status == 'All On')) ? "images/siren-on.png" : this.image = "images/siren-off.png";
        this.onClick = '';
    }
}
Siren.inheritsFrom(BinarySensor);

function Smoke(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = ((item.Status == "Panic") || (item.Status == "On")) ? "images/smoke48on.png" : this.image = "images/smoke48off.png";
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
        this.data = '';
    }
}
Smoke.inheritsFrom(BinarySensor);

function Sound(item) {
	if (arguments.length != 0) {
		this.parent.constructor(item);
		if (item.CustomImage != 0) {
			this.image = "images/" + item.Image + "48_On.png";
		} else {
			this.image = "images/Speaker48_On.png";
		}
		this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
	}
}
Sound.inheritsFrom(UtilitySensor);

function Temperature(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        if (typeof item.Temp != 'undefined') {
            var temp = item.Temp;
            this.image = "images/" + GetTemp48Item(temp);
            if ($.isNumeric(temp)) temp += '\u00B0' + $.myglobals.tempsign;
            this.smallStatus = this.data = temp;
        }
        var pattern = new RegExp('\\d\\s' + $.myglobals.tempsign + '\\b');
        while (this.status.search(pattern) > 0) this.status = this.status.setCharAt(this.status.search(pattern) + 1, '\u00B0');
    }
}
Temperature.inheritsFrom(TemperatureSensor);

function Text(item) {
    if (arguments.length != 0) {
        this.ignoreClick=true;
        this.parent.constructor(item);
        this.imagetext = "";
        this.NotifyLink = this.LogLink = this.onClick = "";
		this.data = item.Data.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
		if (this.data.indexOf("<br />") != -1) {
			this.hasNewLine = true;
		}		
        this.status = this.data;
        this.data = "";
    }
}
Text.inheritsFrom(Sensor);

function Visibility(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.LogLink = this.onClick = "window.location.href = '#/Devices/" + this.index + "/Log'";
    }
}
Visibility.inheritsFrom(WeatherSensor);

function Wind(item) {
    if (arguments.length != 0) {
        this.parent.constructor(item);
        this.image = "images/wind48.png";
        if (typeof item.DirectionStr != 'undefined') {
            this.image = "images/Wind" + item.DirectionStr + ".png";
            this.status = item.DirectionStr;
            this.data = item.DirectionStr;
            this.smallStatus = item.DirectionStr;
            if (typeof item.Direction != 'undefined') {
                this.status = item.Direction + ' ' + item.DirectionStr;
            }
        }
        if (typeof item.Speed != 'undefined') {
            this.status += ', ' + item.Speed + ' ' + $.myglobals.windsign;
            this.data += ' / ' + item.Speed + ' ' + $.myglobals.windsign;
            this.smallStatus = item.Speed + ' ' + $.myglobals.windsign;
        }
        if (typeof item.Gust != 'undefined') {
            this.status += ', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $.myglobals.windsign;
        }
        if (typeof item.Chill != 'undefined') {
            this.status += ', ' + $.t('Chill') + ': ' + item.Chill + '\u00B0' + $.myglobals.tempsign;
        }
    }
}
Wind.inheritsFrom(WeatherSensor);

function Selector(item) {
    if (arguments.length !== 0) {
        this.parent.constructor(item);

        // Selector attributes
        var selector = this;
        this.levelNames = b64DecodeUnicode(item.LevelNames).split('|');
        this.levelInt = item.LevelInt;
        this.levelName = this.levelNames[this.levelInt / 10];
        this.levelOffHidden = item.LevelOffHidden;

        // Device attributes
        this.data = this.levelName;
        this.status = '';
        this.smallStatus = this.status;
        if (item.CustomImage !== 0) {
            this.image = (this.levelName === "Off") ? "images/" + item.Image + "48_Off.png" : "images/" + item.Image + "48_On.png";
        } else {
            this.image = (this.levelName === "Off") ? "images/" + item.TypeImg + "48_Off.png" : "images/" + item.TypeImg + "48_On.png";
        }
        if ((this.levelName === "Off") || (this.levelOffHidden === true)) {
            this.ignoreClick = true;
            this.onClick = 'DoNothing';
        }
        this.useSVGtags = false;

        // Selector methods
        this.onRefreshEvent = function () {
            // Nothing to do yet because status is update correctly
        };
        this.onSelectorValueChange = function (idx, levelname, level) {
            // Send device command
            SwitchSelectorLevel(idx, unescape(levelname), level, this.onRefreshEvent, this.isprotected);
        };
        this.toggleSelectorList = function (selector$) {
            var deviceDetails$ = selector$.parents('.DeviceDetails'),
                list$ = selector$.find('.level-list');
            // Show/Hide underlying controls
            if ((list$.css('display') === 'none') || (list$.css('visibility') !== 'visible')) {
                deviceDetails$
                    .find('#lastseen').hide().end()
                    .find('#twisty').hide().end()
                    .find('#detailsgroup')
                    .find('rect').hide().end()
                    .find('text').hide().end()
                    .end();
            } else {
                deviceDetails$
                    .find('#lastseen').show().end()
                    .find('#twisty').show().end()
                    .find('#detailsgroup')
                    .find('rect').show().end()
                    .find('text').show().end()
                    .end();
            }
            list$.toggle();
        };
        this.onSelectorEvent = function (evt) {
            var type = evt.type,
                target = evt.target,
                target$ = $(target),
                selector$ = target$.parents('.selector'),
                targetTagName = target$.prop('tagName'),
                parent$ = target$.parent(),
                parentTagName = parent$.prop('tagName'),
                text$, idx, levelName, levelInt, deviceDetails$, list$;
            if (type === 'mouseover') {
                parent$.children().addClass("hover");
                return;
            }
            if (type === 'mouseout') {
                parent$.children().removeClass("hover");
                return;
            }
            if (type === 'click') {
                if (parent$.hasClass('current-level')) {
                    selector.toggleSelectorList(selector$);
                    return;
                }
                if (parent$.hasClass('level')) {
                    text$ = parent$.find('text');
                    levelName = text$.text();
                    levelInt = parseInt(text$.data('value'), 10);
                    idx = selector$.data('idx');
                    selector$.find('.current-level text').text(levelName).data('value', levelInt).end();
                    selector.toggleSelectorList(selector$);
                    selector.onSelectorValueChange(idx, levelName, levelInt);
                    return;
                }
                return;
            }
        };
        this.drawCustomStatus = function (elStatus) {
            var idx = this.index,
                levelInt = this.levelInt,
                levelName = this.levelName,
                levelNames = this.levelNames,
                elSVG$, elSelector, elCurrentLevel, elLevelList, elBack, elText, elArrow, elLevel, html,
                selectorLeft = 0, selectorTop = 2, selectorWidth = 203, i,
                p = function (x, y) {
                    return x + " " + y + " ";
                },
                rectangle = function (x, y, w, h, r1, r2, r3, r4) {
                    var strPath = "M" + p(x + r1, y); //A
                    strPath += "L" + p(x + w - r2, y) + "Q" + p(x + w, y) + p(x + w, y + r2); //B
                    strPath += "L" + p(x + w, y + h - r3) + "Q" + p(x + w, y + h) + p(x + w - r3, y + h); //C
                    strPath += "L" + p(x + r4, y + h) + "Q" + p(x, y + h) + p(x, y + h - r4); //D
                    strPath += "L" + p(x, y + r1) + "Q" + p(x, y) + p(x + r1, y); //A
                    strPath += "Z";
                    return strPath;
                };
            elSVG$ = $(getSVGnode());
            if (elSVG$.length === 1) {
                if (elSVG$.find('style.selector-style').length < 1) {
                    html = [];
                    html.push('<style type="text/css" class="selector-style">');
                    html.push('.selector {');
                    html.push('pointer-events: all;');
                    html.push('}');
                    html.push('.selector .background {');
                    html.push('fill: #f5f5f5; stroke-width: 1; stroke: #cccccc;');
                    html.push('}');
                    html.push('.selector .background.hover, .selector .background:hover {');
                    html.push('fill: #0078a3;');
                    html.push('}');
                    html.push('.selector .level-text {');
                    html.push('font-size: 10pt; font-weight: bold; fill: #333333;');
                    html.push('}');
                    html.push('.selector .level-text.hover, .selector .level-text:hover {');
                    html.push('fill: #ffffff;');
                    html.push('}');
                    html.push('.selector .level-list .level path {');
                    html.push('stroke: #000000; stroke-width: 1px; fill: #000000;');
                    html.push('}');
                    html.push('.selector .level-list .level path.hover, .selector .level-list .level path:hover {');
                    html.push('fill: #0078a3;');
                    html.push('}');
                    html.push('.selector .level-list .level text {');
                    html.push('fill: #ffffff; font-weight: normal;');
                    html.push('}');
                    html.push('.selector .level-list .level text.hover, .selector .level-list .level text:hover {');
                    html.push('font-weight: bold;');
                    html.push('}');
                    html.push('.selector .arrow-s {');
                    html.push('fill: #cccccc;');
                    html.push('}');
                    html.push('.selector .arrow-s.hover, .selector .arrow-s:hover {');
                    html.push('fill: #ffffff;');
                    html.push('}');
                    html.push('</style>');
                    elSVG$.prepend(html.join(''));
                }
            }
            elCurrentLevel = makeSVGnode('g', { 'class': 'current-level' }, '');
            elBack = makeSVGnode('rect', { 'class': 'background', 'width': selectorWidth, 'height': 20, 'x': selectorLeft, 'y': selectorTop, 'rx': 6, 'ry': 6 }, '');
            elText = makeSVGnode('text', { 'class': 'level-text', 'x': (selectorLeft + 5), 'y': (selectorTop + 15), 'text-anchor': 'start', 'data-value': levelInt }, levelName);
            elArrow = makeSVGnode('polygon', { 'class': 'arrow-s', 'points': (selectorLeft + selectorWidth - 14) + ',' + (selectorTop + 9) + ' ' + (selectorLeft + selectorWidth - 8) + ',' + (selectorTop + 9) + ' ' + (selectorLeft + selectorWidth - 11) + ',' + (selectorTop + 13) }, '');
            elCurrentLevel.appendChild(elBack);
            elCurrentLevel.appendChild(elText);
            elCurrentLevel.appendChild(elArrow);

            elLevelList = makeSVGnode('g', { 'class': 'level-list', 'style': 'display: none;' }, '');
            i = 0;
            $.each(levelNames, function (index, text) {
                if ((index === 0) && (selector.levelOffHidden === true)) {
                    return;
                }
                var height = 20, width = selectorWidth,
                    x = selectorLeft, y = (selectorTop + 20) + (i * 20),
                    r1 = 0, r2 = 0,
                    r3 = (i === (levelNames.length - 1)) ? 6 : 0, r4 = r3;
                elLevel = makeSVGnode('g', { 'class': 'level' }, '');
                elBack = makeSVGnode('path', { 'class': 'background', 'd': rectangle(x, y, width, height, r1, r2, r3, r4) }, '');
                elText = makeSVGnode('text', { 'class': 'level-text', 'data-value': (index * 10), 'x': (x + 5), 'y': (y + 15), 'text-anchor': 'start' }, text);
                elLevel.appendChild(elBack);
                elLevel.appendChild(elText);
                elLevelList.appendChild(elLevel);
                i += 1;
            });
            elSelector = makeSVGnode('g', { 'class': 'selector', 'data-idx': idx }, '');
            $(elSelector).bind('mouseover mouseout click', this.onSelectorEvent);
            elSelector.appendChild(elCurrentLevel);
            elSelector.appendChild(elLevelList);
            elStatus.appendChild(elSelector);
        };
    }
}
Selector.inheritsFrom(Switch);
