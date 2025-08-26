//A lot of the code below is based on Blindman67's answer to the following question on stackoverflow:
//https://stackoverflow.com/questions/33473456/how-to-detect-and-move-drag-the-free-flow-drawn-lines-in-html-canvas
//Answer permalink: https://stackoverflow.com/a/33481008

var mobile = false;
var mouseUse = true;

(function (a, b) {
	if (/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|mobile.+firefox|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows ce|xda|xiino/i.test(a) || /1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(a.substr(0, 4))) {
		//alert(b);
		//mouseEvent = true;
		mobile = true;
	}
})(navigator.userAgent || navigator.vendor || window.opera, 'This tool is currently not optimized for Mobile Browsers and touch input. Touch support is experimental. Please use a Desktop Browser and a Mouse or Touch Pad for the best experience.');

// https://stackoverflow.com/questions/45108732/drawing-on-html5-canvas-with-support-for-multitouch-pinch-pan-and-zoom
 
var doFor = function doFor(count, callback) {
	var i = 0;
	while (i < count && callback(i++) !== true);
};

var drawMode = false;
var pinchMode = false;
var startup = true;
var touchDelay = 20

const pointer = touch($("#canvas")[0]);

function touch(element) {
	const touch = {
		points: [],
		x: 0,
		y: 0,
		//isTouch : true, // use to determine the IO type.
		count: 0,
//		w: 0,
//		rx: 0,
//		ry: 0,

	}
	var m = touch;
	var t = touch.points;

	function newTouch() {
		for (var j = 0; j < m.pCount; j++) {
			if (t[j].id === -1) {
				return t[j]
			}
		}
	}

	function getTouch(id) {
		for (var j = 0; j < m.pCount; j++) {
			if (t[j].id === id) {
				return t[j]
			}
		}
	}

	function setTouch(touchPoint, point, start, down) {
		if (touchPoint === undefined) {
			return
		}
		if (start) {
			touchPoint.oy = point.pageX;
			touchPoint.ox = point.pageY;
			touchPoint.id = point.identifier;
		} else {
			touchPoint.ox = touchPoint.x;
			touchPoint.oy = touchPoint.y;
		}
		touchPoint.x = point.pageX;
		touchPoint.y = point.pageY;
		touchPoint.down = down;
		if (!down) {
			touchPoint.id = -1
		}
	}

	function mouseEmulator() {
		var tCount = 0;
		for (var j = 0; j < m.pCount; j++) {
			if (t[j].id !== -1) {
				if (tCount === 0) {
					m.x = t[j].x;
					m.y = t[j].y;
				}
				tCount += 1;
			}
		}
		m.count = tCount;
	}

	function touchEvent(e) {
		var i, p;
		p = e.changedTouches;
		if (e.type === "touchstart") {
			for (i = 0; i < p.length; i++) {
				setTouch(newTouch(), p[i], true, true)
			}
		} else if (e.type === "touchmove") {
			for (i = 0; i < p.length; i++) {
				setTouch(getTouch(p[i].identifier), p[i], false, true)
			}
		} else if (e.type === "touchend") {
			for (i = 0; i < p.length; i++) {
				setTouch(getTouch(p[i].identifier), p[i], false, false)
			}
		}
		mouseEmulator();
		e.preventDefault();
		return false;
	}
	touch.pCount = 2
	element = element === undefined ? document : element;
	doFor(2, () => touch.points.push({
		x: 0,
		y: 0,
		//dx: 0,
		//dy: 0,
		down: false,
		id: -1
	}));
	["touchstart", "touchmove", "touchend"].forEach(name => element.addEventListener(name, touchEvent));
	return touch;
}

var mouseIsDown
var img
var dragDots
var helpers = new Helpers();
var lineArray = [];
var selectedLine;
var points;
var drawing = false;
var dragging = false;
var dragOffX;
var dragOffY;
var drawMode = "Draw";
var firstdot = true
var updated = true
var showCross = false
var overCanvas = false
var canvh = 600
var canvw = 1000
var showGrid = false
var canvas = document.getElementById("canvas");
var ctx = canvas.getContext("2d");
ctx.canvas.width  = canvw
ctx.canvas.height = canvh
var mcoords;
var bgimage = false;
var fortyfive = false

var c1
var c2

var mouse = {
	x: 0,
	y: 0,
	button: false,
	lastButton: false,
	which: [false, false, false],
	isTouch: false
};	

customCursors = {
	encoding: "url('data:image/png;base64,",
	drag_small: {
		center: " 25 25,",
		image: "iVBORw0KGgoAAAANSUhEUgAAADMAAAAzCAYAAAA6oTAqAAACQElEQVRoQ+2azW7DIAyAYZdJW6vlVmmnvcLe/yH2CjtN6i1Tu0m9rIMsJIYChmCvCWkuqZSA/fkPQyoF83VWl5RSqJtQd8kpjnVyB4QdiA0GghhvcHuIBcYH8h9A5DAxEG4gUhgN8rzbiY/9Hs1zjpAjg0nxiEtIDUQCMwWEI+SKYfJBzorDFkvloSvAXKZTs92K9nAoXlTJYFwV9YofunyNAEWHQALjU9qETijpA2OK9CkaHLJ8NYumBrzBoMss/sK6wkyHDLRJyp6EKsxyZUc9Y5R62mzE5/GYvB+hhNFVMVV+EMZVKGeVpoYxwYHp4IUp3VhxwehwjwFdwIQUwawC84oTJgZkwaQogRfIvzcA/DCkb1m63Eu9sE4CFqQBxgty+hLi/mHocnMOVyzFf96EuHv1AkKopmlE27YW5wiuDHD6Vvo8Ds/daOlggh7pYMbBqdaEnon9zpmve9ejDwSS0f3IRBgYGqOwF2W0dysEKWCskO4dkz1vbADMF9PaQ6OF8qBECT1ndZ6pJ2eMa6upZlGg/mFunF91ncGAFtcBxIDmApPVm4WA5gCD6bCO/Qz0EFzMFrvTnLoip3TfKUbJlb+uA41c60S7cPUQS+Ip8syYm2eg9dzjoMFK/edy19KxTqI0j4o9Y5LdVXqxXwFy+zYXfHbfZ9IPKWb85QyrXlh1oqxuxTmDdduJ22sSPUgmgUBV/A8gx0OUoWX1jVhMT3leVW8WKgpcHmFtZ3whxw2iZZIWAF9IOod/rPJ+AQ3iOFgpekFcAAAAAElFTkSuQmCC')"
	},
}

function endDrawingAndDeselct(p){
	//TODO: cleanup
	if(p !== undefined){
		if(p.type == "mousedown"){
			if(mouse.which[2]){
				firstdot = true
				if(lineArray[helpers.currentID()-1].line.length == 1){
					lineArray[helpers.currentID()-1].numLEDs = 1
				}
				if(!lineArray[helpers.currentID()-1].done){
					if(autoid.checked){
						newsindex.value = parseInt(newsindex.value) + lineArray[helpers.currentID()-1].numLEDs
					}
					lineArray[helpers.currentID()-1].done = true
				}
				updated = true
			}
		}
	}else {
		firstdot = true
		if(lineArray[helpers.currentID()-1].line.length == 1){
			lineArray[helpers.currentID()-1].numLEDs = 1
		}
		if(!lineArray[helpers.currentID()-1].done){
			if(autoid.checked){
				newsindex.value = parseInt(newsindex.value) + lineArray[helpers.currentID()-1].numLEDs
			}
			lineArray[helpers.currentID()-1].done = true
		}
		updated = true
	}
}

$("document").ready(function(){
	mouseEnterOrLeaveEvent({type: "mouseenter"})
	var interval = false
	newnleds.value = 10;
	newsindex.value = 0;
	autoid.checked = false
	dotmode.checked = false
	grid.checked = false
	precision.checked = false
	exp.value = ""
	stretchorextend.checked = false
	$("#canvwinput").val(canvw)
	$("#canvhinput").val(canvh)

$(".checkbox").on("click touchstart", function(){
  var $this = $(this),
      $checkbox = $this.find("input[type=checkbox]");
  
  if($checkbox.is(":checked")){
    $this.removeClass("checked");
    $checkbox.prop("checked", false);
  } else {
    $this.addClass("checked");
    $checkbox.prop("checked", true);
  }
  $checkbox.trigger("change")
  return false;
});

	jQuery("#rotateLeft").on('mousedown touchstart', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = true;
			interval = setInterval(function() {
				if(mouseIsDown) {
					rotate(-1)
				}
			}, 50);
		}
	});
	
	jQuery("#rotateLeft").on('touchend', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = false;
			clearInterval(interval)
		}
	});
	
	jQuery("#rotateRight").on('mousedown touchstart', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = true;
			interval = setInterval(function() {
				if(mouseIsDown) {
					rotate(1)
				}
			}, 50);
		}
	});
	
	jQuery("#rotateRight").on('touchend', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = false;
			clearInterval(interval)
		}
	});
	
	jQuery("#scaleUp").on('mousedown touchstart', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = true;
			interval = setInterval(function() {
				if(mouseIsDown) {
					scaleUpDown(1.01)
				}
			}, 50);
		}
	});
	
	jQuery("#scaleUp").on('touchend', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = false;
			clearInterval(interval)
		}
	});

	jQuery("#scaleDown").on('mousedown touchstart', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = true;
			interval = setInterval(function() {
				if(mouseIsDown) {
					scaleUpDown(1/1.01)
				}
			}, 50);
		}
	});

	jQuery("#scaleDown").on('touchend', function(event) {
		event.preventDefault();
		if(selectedLine !== undefined){
			mouseIsDown = false;
			clearInterval(interval)
		}
	});

	jQuery("#delete").on('click', function() {
		deleteLine()
	});
	
	jQuery("#clone").on('click', function() {
		cloneLine()
	});

	jQuery(window).on('mouseup', function() {
		mouseIsDown = false;
		clearInterval(interval)
	});
	
	jQuery("#dotmode").on('click touchstart', function() {
		firstdot = this.checked
		selectedLine = undefined
		selectedDot = undefined
	})
	
	jQuery("#test4").on("click", function(){
		updated = true;
	})
	
	jQuery("#grid").on('change', function() {
		div = 25
		showGrid = this.checked
		if(showGrid){ 
			$("#canvwinput").val(parseInt($("#canvwinput").val()/div))
			$("#canvhinput").val(parseInt($("#canvhinput").val()/div))
			resizeCanvas(true, true)
			updated = true

			lineArray.each(function(p, i) {
				p.line.each(function(q, i) {
					q.x = (q.x - canvas.width /2)/div + canvas.width /2
					q.y = (q.y - canvas.height/2)/div + canvas.height/2

				})
				segment(p)
				updateExtent(p)
			})
			
		} else {
			updated = true
			$("#canvwinput").val(parseInt($("#canvwinput").val()*div))
			$("#canvhinput").val(parseInt($("#canvhinput").val()*div))
			
			lineArray.each(function(p,i) {
				p.line.each(function(q,i) {
					q.x = (q.x - canvas.width /2) * div + canvas.width/2
					q.y = (q.y - canvas.height/2) * div + canvas.height/2
				})
				segment(p)
				updateExtent(p)
			})
			resizeCanvas(true, true)
		}
	})
	
	jQuery("#dragdots").on('click', function() {
		dragDots = this.checked
	})
	
	//if(mobile){
	//	$(canvas).on('touchstart touchmove touchend', touchEvent);
	//} else {
		jQuery("#canvas").on('mousemove mousedown mouseup mouseout touchstart touchmove touchend', mouseEvent);
	//}
	
	jQuery("#canvas").on('mousedown', function(p) {
		endDrawingAndDeselct(p)
	})
	
	jQuery("#nleds, #sindex").on("keyup", function(){
		saveProps()
	})	
	
	jQuery("#canvwinput, #canvhinput").on("keyup", function(){
		if(parseInt(canvwinput.value)<10 || parseInt(canvwinput.value)<10){
			
		} else {
			resizeCanvas(true)
		}
	})

	$("#fileInput").on("change", function(event) {
		file = event.target.files;
		var files = event.target.files;
		for (var i = 0, ln = files.length; i < ln; i++) {
			var file = files[i];
			if (/image/.test(file.type)) {
				var URLObj = window.URL || window.webkitURL;
				var source = URLObj.createObjectURL(file);
				img = new Image();
				img.onload = function() {
					$("#canvwinput").val(img.width)
					$("#canvhinput").val(img.height)
					resizeCanvas(true)
					bgimage = true
					fileResetTrigger.disabled = false
				}
				img.src = source;
			}
		}
	});

	$("#fileResetTrigger").on("click", function(event) {
		event.preventDefault();
		img = undefined
		resizeCanvas()
		updated = true
		bgimage = false
		fileResetTrigger.disabled = true
		$("#fileInput").val("")
	})
		
	$(canvas).on('mouseenter mouseleave', mouseEnterOrLeaveEvent);
	
	$(canvas).on("contextmenu", function(e) {
		e.preventDefault();
	}, false);
	
	canvww = parseInt(canvwinput.value)
	canvhh = parseInt(canvhinput.value)
	window.addEventListener('resize', debounce(resizeCanvas, 250))
	resizeCanvas()

	function handleKeyDown(event) {
		const key = event.keyCode;
		if (key === 38) {
			selectedLine.line.forEach((point) => {
				point.y -= 1;
			});
		} else if (key === 40) {
			selectedLine.line.forEach((point) => {
				point.y += 1;
			});
		} else if (key === 37) {
			selectedLine.line.forEach((point) => {
				point.x -= 1;
			});
		} else if (key === 39) {
			selectedLine.line.forEach((point) => {
				point.x += 1;
			});
		} else if (key === 46) {
			deleteLine()
			return
		} else if (key === 27) {
			selectedLine = undefined
			updated = true
			return
		} else if (key === 67) {
			if(!fortyfive){
				fortyfiveorigin = {x:mouse.x, y:mouse.y}
				pointOnGuide    = fortyfiveorigin
				fortyfive1      = {x1:mouse.x-100000, y1:mouse.y+100000,x2:mouse.x+100000, y2:mouse.y-100000}
				fortyfive2      = {x1:mouse.x+100000, y1:mouse.y+100000,x2:mouse.x-100000, y2:mouse.y-100000}
				fortyfive3      = {x1:mouse.x-100000, y1:mouse.y-1,x2:mouse.x+100000, y2:mouse.y-1}
				fortyfive4      = {x1:mouse.x, y1:mouse.y+100000,x2:mouse.x, y2:mouse.y-100000}
			}
			fortyfive = true
			
			updated = true
			return
		} else {
			fortyfive = false
			updated = true
			return
		}
		
		updateExtent(selectedLine)
		segment(selectedLine);
		updated = true
	}
	function handleKeyUp(event) {
		fortyfive = false
		updated = true
	}
	document.addEventListener("keydown", handleKeyDown);
	document.addEventListener("keyup",   handleKeyUp);
})

function rotate(angle){
	for (i = 0; i < selectedLine.line.length; i++) {
		selectedLine.line[i] = rotatePoint(selectedLine.line[i].x, selectedLine.line[i].y, selectedLine.extent.pivotX, selectedLine.extent.pivotY, Math.PI / 180 * angle)
	}
	updateExtent()
	segment(selectedLine)
	updated = true
}

function shrinkGrid(){
	map = eval(exp.value)
	let maxX = map[0][0];
	let maxY = map[0][1];
	let minX = map[0][0];
	let minY = map[0][1];
	
	for (let i = 1; i < map.length; i++) {
		if (map[i][0] > maxX) {
			maxX = map[i][0];
		}
		if (map[i][1] > maxY) {
			maxY = map[i][1];
		}
		if (map[i][0] < minX) {
			minX = map[i][0];
		}
		if (map[i][1] < minY) {
			minY = map[i][1];
		}
	}
	map.each(function(p){
		p[0] = p[0] - minX
		p[1] = p[1] - minY
	})
	exp.value = JSON.stringify(map)
}

function resizeCanvas(gridchange, coarse){
	
	if(gridchange === undefined && coarse == undefined){
		stretch = false
	}
	if(gridchange === true && coarse == undefined){
		stretch = stretchorextend.checked
	}
	if(gridchange === true && coarse == true){
		stretch = false
	}

	oldcanvww = canvww
	oldcanvhh = canvhh
	
	prevWidth  = parseInt(ctx.canvas.style.width,  10)
	prevHeight = parseInt(ctx.canvas.style.height, 10)
	
	canvww = parseInt(canvwinput.value)
	canvhh = parseInt(canvhinput.value)
	gridsizechangeratioV = 1
	gridsizechangeratioH = 1
	
	
	if(gridchange !== undefined){
		if(gridchange){
			gridsizechangeratioV = oldcanvww/canvww
			gridsizechangeratioH = oldcanvhh/canvhh
		}
	}	
	
	//alert(window.innerWidth)
	
	maxW = jQuery("#tabs-1").width() - 10
	maxH = window.innerHeight - 365
	
	if(canvww > maxW || canvhh > maxH){
		scaleF = Math.max(canvww/maxW, canvhh/maxH)
		canvas.width =  canvww / scaleF
		canvas.height = canvhh / scaleF
		canvas.style.width =  (canvww / scaleF) + "px"
		canvas.style.height = (canvhh / scaleF) + "px"
	} else {
		canvas.width = canvww
		canvas.height = canvhh
		canvas.style.width = (canvww) + "px"
		canvas.style.height = (canvhh) + "px"
	}

	if(canvww < maxW && canvhh < maxH){
		scaleF = Math.max(canvww/maxW, canvhh/maxH)
		canvas.width =  Math.round(canvww / scaleF)
		canvas.height = Math.round(canvhh / scaleF)
		canvas.style.width = (canvww / scaleF) + "px"
		canvas.style.height = (canvhh / scaleF) + "px"
	}
	
	newWidth  = parseInt(ctx.canvas.style.width,  10)
	newHeight = parseInt(ctx.canvas.style.height, 10)
	ratioV = newWidth/prevWidth   * gridsizechangeratioV
	ratioH = newHeight/prevHeight * gridsizechangeratioH

	rect = canvas.getBoundingClientRect()
	scale = canvw / (rect.width - 4)
	
	lineArray.each(function(p,i) {
		p.line.each(function(q,i) {
			q.x = (q.x * ratioV + (canvww-oldcanvww)/2/scaleF)*(stretch ? 1/gridsizechangeratioV : 1)-(stretch ? (canvww/oldcanvww/2-0.5) * canvas.width : 0)
			q.y = (q.y * ratioH + (canvhh-oldcanvhh)/2/scaleF)*(stretch ? 1/gridsizechangeratioH : 1)-(stretch ? (canvhh/oldcanvhh/2-0.5) * canvas.height : 0)
		})
		helpers.resetExtent();
		p.line.each(function(p) {
			helpers.extent(p);
		})
		p.extent = helpers.copyOfExtent(p);
		segment(p)
	})
	updated = true
}

function debounce(func, delay) {
	let timer;
	return function() {
		clearTimeout(timer);
		timer = setTimeout(func, delay);
	}
}

function cross(e){
	const x = mouse.x - 2
	const y = mouse.y - 1
	ctx.strokeStyle = "gray";
	ctx.lineWidth = 1;
	ctx.beginPath();
	ctx.moveTo(0, y);
	ctx.lineTo(canvas.width, y);
	ctx.moveTo(x, 0);
	ctx.lineTo(x, canvas.height);
	ctx.stroke();
	
	mcoords = "Cursor x: " + Math.round(mouse.x * scale) + " y: " + Math.round(mouse.y * scale)
	ctx.font = "15px Verdana";
	ctx.fillStyle = "red";
	cx = canvas.width - 190
	cy = 20
	ctx.fillText(mcoords, cx, cy);
}

function fortyfivers(e) {
	// Get the position of the mouse cursor.
	const x = fortyfiveorigin.x - 2;
	const y = fortyfiveorigin.y - 1;
	
	let min = Math.min(canvas.width, canvas.height)
	let hyp = Math.sqrt((min**2)*2);

	ctx.strokeStyle = "gray";
	ctx.lineWidth = 1;

	ctx.beginPath();
	ctx.moveTo(0, y);
	ctx.lineTo(canvas.width, y);
	ctx.moveTo(x, 0);
	ctx.lineTo(x, canvas.height);
	ctx.stroke();

	// Draw the rotated cross.

	ctx.beginPath();
	ctx.translate(x+min, y-min);
	ctx.rotate(45 * Math.PI / 180);
	ctx.moveTo(0, 0);
	ctx.lineTo(0, hyp*2);
	ctx.stroke();
	ctx.setTransform(1, 0, 0, 1, 0, 0);

	ctx.beginPath();
	ctx.translate(x+min, y+min);
	ctx.rotate(-45 * Math.PI / 180);
	ctx.moveTo(0, 0);
	ctx.lineTo(0, -1*hyp*2);
	ctx.stroke();
	ctx.setTransform(1, 0, 0, 1, 0, 0);
  }

function deleteLine(){
	if(firstdot){
		var toBeDeleted
		if(selectedLine !== undefined){
			lineArray.each(function(p,i) {
				if(p.id == selectedLine.id){
					selectedLine.hidden = true
					if(selectedLine.gridPoint){
						xx = Math.floor(selectedLine.line[0].x/(canvas.width/ parseInt(canvwinput.value)))
						yy = Math.floor(selectedLine.line[0].y/(canvas.height/parseInt(canvhinput.value)))
					}
				}
			})
		}
		drawMode = "Draw"
		selectedLine = undefined
		selectedDot = undefined
		updated = true
		loadProps()
		jQuery("#props").hide()
	}
}

function cloneLine(){
	var toBeCloned
	if(selectedLine !== undefined){
		lineArray.each(function(p,i) {
			if(p.id == selectedLine.id){
				toBeCloned = i
			}
		})
	}
	if(toBeCloned !== undefined){
		var j = lineArray.push(JSON.parse(JSON.stringify(lineArray[toBeCloned])))
		
		lineArray[j-1].line.each(function(p,k) {
			p.x += 30
			p.y += 30
		})

		if(autoid.checked){
			var start = 0
			lineArray.each(function(obj) {
				if (obj.startIndex + obj.numLEDs > start) {
					start = obj.startIndex + obj.numLEDs;
				}
			});
			lineArray[j-1].startIndex = start
		}

		lineArray[j-1].id = helpers.getID()
		
		helpers.resetExtent();
		lineArray[j-1].line.each(function(p) {
			helpers.extent(p);
		})
		lineArray[j-1].extent = helpers.copyOfExtent(lineArray[j-1]);
		segment(lineArray[j-1])
	}
	updated = true
}

function save(){
	const content = JSON.parse(JSON.stringify(lineArray.filter(obj => obj.hidden == false)));
	for (i = 0; i < content.length; i++) {
		for (j = 0; j < content[i].line.length; j++) {
			content[i].line[j].x = Math.round(content[i].line[j].x * scale)
			content[i].line[j].y = Math.round(content[i].line[j].y * scale)
		}
	}
	const file = new Blob([JSON.stringify(content)], {type: 'text/plain'});
	const fileURL = URL.createObjectURL(file);
	const a = document.createElement('a');
	a.href = fileURL;
	a.download = 'map.txt';
	document.body.appendChild(a);
	a.click();
	document.body.removeChild(a);
	URL.revokeObjectURL(fileURL);
}

function centerCoordinates(coordinates, centerX, centerY) {
    // Find the current centroid (average x and y values)
    let sumX = 0;
    let sumY = 0;
    
    coordinates.forEach(point => {
        sumX += point[0];
        sumY += point[1];
    });
    
    const centroidX = sumX / coordinates.length;
    const centroidY = sumY / coordinates.length;
    
    // Calculate the offset to center around the given (centerX, centerY)
    const offsetX = centerX - centroidX;
    const offsetY = centerY - centroidY;
    
    // Apply the offset to each coordinate
    const centeredCoordinates = coordinates.map(point => [
        point[0] + offsetX,
        point[1] + offsetY
    ]);
    
    return centeredCoordinates;
}

function validateFileContent(content) {
    // Split the content by lines
    const lines = content.split('\n');
    
    // Define a regular expression to match <number>,<number> with optional whitespace
    const regex = /^\s*-?\d+(\.\d+)?\s*,\s*-?\d+(\.\d+)?\s*$/;

    // Check each line against the format
    var cnt = 0
	for (let line of lines) {
		cnt++;
        line = line.trim(); // Remove whitespace at the beginning and end of the line
        if (line && !regex.test(line)) {
			alert("Invalid format on line " + cnt + ": \"" + line + "\"");
			throw new Error(`Invalid format on line ${cnt}: "${line}"`);
        }
    }
}

function load(json){
	var imparr;
	try {
		lineArray = JSON.parse(json)
		lineArray.splice(0);
		selectedLine = undefined
		helpers.id = undefined
		lineArray = JSON.parse(json)
		
		for (i = 0; i < lineArray.length; i++) {
			for (j = 0; j < lineArray[i].line.length; j++) {
				lineArray[i].line[j].x = Math.round(lineArray[i].line[j].x / scale)
				lineArray[i].line[j].y = Math.round(lineArray[i].line[j].y / scale)
			}
		}
		
		lineArray.each(function(p) {
			segment(p)
		})
		
		helpers.id = lineArray.reduce((acc, obj) => obj.id > acc ? obj.id : acc, -Infinity);
		updated = true
	} catch (e) {
		validateFileContent(json)
		imparr = json.trim().split('\n').map(line => line.split(',').map(Number));
		imparr =  centerCoordinates(imparr,$("#canvwinput").val()/2,$("#canvhinput").val()/2)
		//$("#canvwinput").val(result.maxValues.x)
		//$("#canvhinput").val(result.maxValues.y)
		//resizeCanvas(true, undefined)
		for (ii = 0; ii < imparr.length; ii++) {
			update(1, (imparr[ii][0] / scale ) + 2, (imparr[ii][1] / scale ) + 2, 1)
			update(1, (imparr[ii][0] / scale ) + 2, (imparr[ii][1] / scale ) + 2, 2)
		}
		clickEvent = new MouseEvent('mouseleave', {});
		mouseEvent(clickEvent)
		return
	}	
}

function exportMatrix() {
	var expTxt = ""
	lineArray.sort((a, b) => a.startIndex - b.startIndex);
	for (i = 0; i < lineArray.length; i++) {
		if(lineArray[i].hidden == false){
			for (j = 0; j < lineArray[i].LEDpositions.length; j++) {
				if(precision.checked)
					expTxt = expTxt + (lineArray[i].startIndex + j) + "\t" + Number((lineArray[i].LEDpositions[j].x / canvas.width*canvww).toFixed(2)) + "\t" + Number((lineArray[i].LEDpositions[j].y / canvas.height*canvhh).toFixed(2)) + "\n"
				else
					expTxt = expTxt + (lineArray[i].startIndex + j) + "\t" + Math.round(lineArray[i].LEDpositions[j].x / canvas.width*canvww,0) + "\t" + Math.round(lineArray[i].LEDpositions[j].y / canvas.height*canvhh,0) + "\n"

				//expTxt = expTxt + (lineArray[i].startIndex + j) + "\t" + (lineArray[i].LEDpositions[j].x / canvas.width*canvww) + "\t" + (lineArray[i].LEDpositions[j].y / canvas.height*canvhh) + "\n"

			}
		}
	}
	generatePixelblazeMap(parseCoordinatesText(expTxt).leds)
}

function drawBG(){
	if(img !== undefined){
		ctx.filter = 'opacity(.5) grayscale(1)';
		
		const canvasWidth = canvas.width;
		const canvasHeight = canvas.height;
		const imageWidth = img.width;
		const imageHeight = img.height;

		const canvasAspectRatio = canvasWidth / canvasHeight;
		const imageAspectRatio = imageWidth / imageHeight;

		let drawWidth = canvasWidth;
		let drawHeight = canvasHeight;

		if (canvasAspectRatio > imageAspectRatio) {
			drawWidth = imageWidth * (canvasHeight / imageHeight);
		} else if (canvasAspectRatio < imageAspectRatio) {
			drawHeight = imageHeight * (canvasWidth / imageWidth);
		}

		if(parseInt(canvwinput.value) > imageWidth && parseInt(canvhinput.value) > imageHeight){
			drawWidth  = imageWidth  / Math.max((parseInt(canvwinput.value) / imageWidth),(parseInt(canvhinput.value) / imageHeight) * scale)
			drawHeight = imageHeight / Math.max((parseInt(canvwinput.value) / imageWidth),(parseInt(canvhinput.value) / imageHeight) * scale)
		}

		const x = (canvasWidth - drawWidth) / 2;
		const y = (canvasHeight - drawHeight) / 2;

		ctx.drawImage(img, x, y, drawWidth, drawHeight);
		ctx.filter = 'none';
	}
}

//The following two function have been taken from Jason Coon's awesome led-mapper tool. 
//https://github.com/jasoncoon/led-mapper

function generatePixelblazeMap(leds) {
	if(precision.checked)
		var map = leds.map((led) => `[${led.x.toFixed(2)},${led.y.toFixed(2)}]`).join(",");
	else 
		var map = leds.map((led) => `[${led.x},${led.y}]`).join(",");
	if(map == ""){
		exp.value = "";
	} else {
		exp.value = `[${map}]`;
	}
}

function parseCoordinatesText(text) {
	const lines = text?.split("\n");
	const rows = lines.map((line) => {
		const columns = line.split("\t");
		return columns.map((s) => parseFloat(s));
	});

	const leds = [];

	let minX, minY, maxX, maxY, width, height;

	minX = minY = 1000000;
	maxX = maxY = -1000000;

	for (const row of rows) {
		const index = parseInt(row[0]);
		const x = row[1];
		const y = row[2];

		if (isNaN(index) || isNaN(x) || isNaN(y)) continue;

		if (x < minX) minX = x;
		if (x > maxX) maxX = x;

		if (y < minY) minY = y;
		if (y > maxY) maxY = y;

		leds.push({
			index,
			x,
			y,
		});
	}

	width = maxX - minX + 1;
	height = maxY - minY + 1;

	return {
		height,
		leds,
		maxX,
		maxY,
		minX,
		minY,
		rows,
		width,
	};
}


function copyToClipboard() {
	var copyText = document.getElementById("exp");
	copyText.select();
	copyText.setSelectionRange(0, 999999);
	navigator.clipboard.writeText(copyText.value);
	copyText.blur();
	$("#copied").show()
	$("#exp").css("background-color", "#50c878").animate({
		"background-color": "transparent",
	}, 300, function() {
	});
} 


function updateExtent(p) {
	var line = p
	if(line === undefined){
		line = selectedLine
	}
	helpers.resetExtent();
	line.line.each(function(p) {
		helpers.extent(p);
	})
	line.extent = helpers.copyOfExtent(line);
}

function flipLEDOrder() {
	if(selectedLine !== undefined){
		selectedLine.line.reverse()
		segment(selectedLine)
		updated = true
	}
}

function setCursor(name) {
	if (name === undefined) {
		canvas.style.cursor = "default";
	}
	if (customCursors[name] !== undefined) {
		var cur = customCursors[name];
		canvas.style.cursor = customCursors.encoding + cur.image + cur.center + " pointer";
	} else {
		canvas.style.cursor = name;
	}
}

function mouseEnterOrLeaveEvent(event) {
	if (event.type === "mouseenter") {
		//console.log("me")
		overCanvas = true;
		showCross = true
	}else{
		//console.log("ml")
		overCanvas = false;
		showCross = false
	}
}

function mouseEvent(event) {
	updated = true
	if(event.type === "touchstart" || event.type === "touchend" || event.type === "touchmove"){
		obj = {
			x: (event.changedTouches[0].clientX - rect.left),
			y: (event.changedTouches[0].clientY - rect.top),
			isTouch: true
		}
	} else {
		obj = {
			x: (event.clientX - rect.left),
			y: (event.clientY - rect.top),
			isTouch: false
		}
	}
	//console.log("me")
	//console.log(event.button)
	mouse.x = Math.round(obj.x);
	mouse.y = Math.round(obj.y);
	mouse.isTouch = obj.isTouch;
	
	if (event.type === "mouseout") {
		mouse.button = false;
		mouse.which[0] = false;
		mouse.which[1] = false;
		mouse.which[2] = false;
	}
	if (event.type === "mousedown") {
		mouse.button = true;
		mouse.which[event.button] = true;
	}
	if (event.type === "mouseup") {
		mouse.button = false;
		mouse.which[event.button] = false;
	}
	if (event.type === "touchstart") {
		mouse.button = true;
		mouse.which[0] = true;
	}
	if (event.type === "touchend") {
		mouse.button = false;
		mouse.which[0] = false;
	}
	event.preventDefault();
}

if (Array.prototype.each === undefined) {
	Object.defineProperty(Array.prototype, 'each', {
		writable: false,
		enumerable: false,
		configurable: false,
		value: function(func) {
			var i, returned;
			var len = this.length;
			for (i = 0; i < len; i++) {
				returned = func(this[i], i);
				if (returned !== undefined) {
					this[i] = returned;
				}
			}
		}
	});
}

function Helpers() {}

Helpers.prototype.resetExtent = function() {
	if (this.extentObj === undefined) {
		this.extentObj = {};
	}
	this.extentObj.minX = Infinity;
	this.extentObj.minY = Infinity;
	this.extentObj.maxX = -Infinity;
	this.extentObj.maxY = -Infinity;
}

Helpers.prototype.extent = function(p) {
	this.extentObj.minX = Math.min(this.extentObj.minX, p.x);
	this.extentObj.minY = Math.min(this.extentObj.minY, p.y);
	this.extentObj.maxX = Math.max(this.extentObj.maxX, p.x);
	this.extentObj.maxY = Math.max(this.extentObj.maxY, p.y);
}

Helpers.prototype.copyOfExtent = function(p) {
	var pivot = getLineMidpoint(p.line)
	var obj =	{
		minX: this.extentObj.minX - 7,
		minY: this.extentObj.minY - 7,
		maxX: this.extentObj.maxX + 7,
		maxY: this.extentObj.maxY + 7,
		centerX: (this.extentObj.maxX - this.extentObj.minX) / 2,
		centerY: (this.extentObj.maxY - this.extentObj.minY) / 2,
		width: this.extentObj.maxX - this.extentObj.minX + 14,
		height: this.extentObj.maxY - this.extentObj.minY + 14,
		pivotX: pivot.x,
		pivotY: pivot.y,
	};
	return obj
}

Helpers.prototype.getID = function() {
	if (this.id === undefined) {
		this.id = 0;
	}
	this.id += 1;
	return this.id;
}

Helpers.prototype.currentID = function() {
	return this.id;
}

Helpers.prototype.closestPointOnLine = function(x, y, x1, y1, x2, y2) {
	var px = x2 - x1;
	var py = y2 - y1;
	//console.log(px,py)
	var u = this.lineSegPos = Math.max(0, Math.min(1, ((x - x1) * px + (y - y1) * py) / (this.distSqr1 = (px * px + py * py))));
	
	//TODO: clean that up, not needed
	//if(u > 0 && u < 1){
	var x = x1 + u * (x2 - x1);
	var y = y1 + u * (y2 - y1);
	closest = { x: x, y: y };
	//}
	return closest;
}

function redrawAll() {
	ctx.clearRect(0, 0, canvas.width, canvas.height);
	ctx.globalCompositeOperation = "source-over"
	if (img) {
		drawBG()
	}else{
		ctx.clearRect(0, 0, canvas.width, canvas.height);
	}
	drawGrid()
	if(points !== undefined){
		if(points.length > 0){
			for (i = 1; i < points.length; i++) {
				var p1 = points[i - 1];
				var p2 = points[i];
				ctx.lineWidth = 1;
				ctx.strokeStyle = "black";
				ctx.beginPath();
				ctx.moveTo(p1.x, p1.y);
				ctx.lineTo(p2.x, p2.y);
				ctx.stroke();
			}
		}
	}

	lineArray.each(function(p) {
		if(selectedLine === undefined){
			redraw(p);
		} else if(p.id != selectedLine.id){
			redraw(p);
		}
	})
	if(selectedLine !== undefined){
		redraw(selectedLine);
	}

	if(showCross){
		cross()
	}

	if(fortyfive){
		ppp = helpers.closestPointOnLine(mouse.x, mouse.y, fortyfive1.x1, fortyfive1.y1, fortyfive1.x2, fortyfive1.y2)
		qqq = helpers.closestPointOnLine(mouse.x, mouse.y, fortyfive2.x1, fortyfive2.y1, fortyfive2.x2, fortyfive2.y2)
		rrr = helpers.closestPointOnLine(mouse.x, mouse.y, fortyfive3.x1, fortyfive3.y1, fortyfive3.x2, fortyfive3.y2)
		sss = helpers.closestPointOnLine(mouse.x, mouse.y, fortyfive4.x1, fortyfive4.y1, fortyfive4.x2, fortyfive4.y2)

		var dist1 = calculateDistance(mouse.x, mouse.y, ppp)
		var dist2 = calculateDistance(mouse.x, mouse.y, qqq)
		var dist3 = calculateDistance(mouse.x, mouse.y, rrr)
		var dist4 = calculateDistance(mouse.x, mouse.y, sss)

		var dist5 = calculateDistance(mouse.x, mouse.y, fortyfiveorigin)
		if(dotmode.checked){
			if(dist5 < 20){
				pointOnGuide = fortyfiveorigin
				ctx.fillStyle = "hsl(0 0% 0%)";
				ctx.fillRect(pointOnGuide.x-5, pointOnGuide.y-4, 7, 7);
			} else {
				if(Math.min(dist1, dist2, dist3, dist4) < 20){
					if(dist1<Math.min(dist2,dist3,dist4)){
						pointOnGuide = ppp
						ctx.fillStyle = "hsl(0 0% 0%)";
						ctx.fillRect(ppp.x-5, ppp.y-4, 7, 7);
					}else if(dist2<Math.min(dist1,dist3,dist4)){
						pointOnGuide = qqq
						ctx.fillStyle = "hsl(0 0% 0%)";
						ctx.fillRect(qqq.x-5, qqq.y-4, 7, 7);
					}else if(dist3<Math.min(dist1,dist2,dist4)){
						pointOnGuide = rrr
						ctx.fillStyle = "hsl(0 0% 0%)";
						ctx.fillRect(rrr.x-5, rrr.y-4, 7, 7);
					}else if(dist4<Math.min(dist1,dist2,dist3)){
						pointOnGuide = sss
						ctx.fillStyle = "hsl(0 0% 0%)";
						ctx.fillRect(sss.x-5, sss.y-4, 7, 7);
					}
				} else {
					pointOnGuide = fortyfiveorigin
				}
			}
		}
	}

	if(fortyfive){
		fortyfivers()
	}

	errstring = ""
	coords = ""
	
	lineArray.each(function(p) {
		if(p.line !== undefined){
			for (i = 0; i < p.line.length; i++) {
				if(p.line[i].x > canvas.width || p.line[i].x < 0 || p.line[i].y > canvas.height || p.line[i].y < 0){
					errstring = "Warning: LEDs outside of the visible area!<br>"
					break
				}
			}
		}
	})
	
	if (checkOverlap(lineArray)) {
		errstring = errstring + "Warning: Indices overlap!"
	}
}	

function clean(line) {
	//return
	var x = -1;
	var y = -1;
	var _x = -1;
	var _y = -1;
	for (i = 0; i < line.length; i++) {
		_x = line[i].x
		_y = line[i].y
		if (x == line[i].x && y == line[i].y) {
			line[i].x = -1
			line[i].y = -1
		}
		x = _x
		y = _y
	}
	
	for (i = line.length; i > 0; i--) {
		if (line[i - 1].x == -1 && line[i - 1].y == -1) {
			line.splice(i - 1, 1)
		}
	}

	if(measureLength(line) < 10){
		line.length = 1;
	}
}

function measureLength(line) {
	totalLen = -1
	x1 = -1
	y1 = -1

	for (i = 0; i < line.length; i++) {
		if (i == 0) {
			x1 = line[i].x
			y1 = line[i].y
		} else {
			a = x1 - line[i].x
			b = y1 - line[i].y
			x1 = line[i].x
			y1 = line[i].y
			len = Math.sqrt(a * a + b * b)
			totalLen = totalLen + len
		}
	}
	return totalLen
}

function simplify(lineDesc) {
	x1 = -1
	y1 = -1
	
	var line = lineDesc.line
	
	for (i = 0; i < line.length; i++) {
		if (i == 0) {
			x1 = line[i].x
			y1 = line[i].y
		} else {
			if (i + 1 != line.length) {
				a = x1 - line[i].x
				b = y1 - line[i].y
				len = Math.sqrt(a * a + b * b)
				if (len < 20 * Math.pow(2, 0)) {
					line[i].x = -1
					line[i].y = -1
					continue
				}
				x1 = line[i].x
				y1 = line[i].y
			}
		}
	}
	
	for (i = line.length; i > 0; i--) {
		if (line[i - 1].x == -1 && line[i - 1].y == -1) {
			line.splice(i - 1, 1)
		}
	}
	

	
	if(line.length > 1){
		if (Math.sqrt((line[line.length - 1].x - line[line.length - 2].x) ** 2 + (line[line.length - 1].y - line[line.length - 2].y) ** 2) < 10) {
			line.splice(line.length - 2, 1);
		}
		if(lineDesc.numLEDs < 2){
			lineDesc.numLEDs = 2
		}
	}
}

function segment(lineDesc) {
	if (lineDesc.numLEDs < 1 || isNaN(lineDesc.numLEDs) || lineDesc.startIndex < 0 || isNaN(lineDesc.startIndex)) {
		lineDesc.LEDpositions = []
		return
	}
	segments = lineDesc.numLEDs - 1
	segmentCounter = 1
	curLen = -1
	var line = lineDesc.line;
	x1 = -1
	y1 = -1
	totalLen = measureLength(lineDesc.line)

	var LEDpositions = []

	LEDpositions.push({
		x: line[0].x,
		y: line[0].y
	})
		
	if(line.length > 1){
		lineDesc.isLine = true
		lineDesc.isDot = false
		for (i = 0; i < line.length; i++) {
			if (segmentCounter < segments) {
				if (i == 0) {
					x1 = line[i].x
					y1 = line[i].y
				} else {
					a = x1 - line[i].x
					b = y1 - line[i].y
					x1 = line[i].x
					y1 = line[i].y
					len = Math.sqrt(a * a + b * b)
					curLen = curLen + len
					if (curLen > totalLen / segments * segmentCounter) {
						overstep = curLen - (totalLen / segments * segmentCounter)
						Angle = Math.atan2(line[i - 1].y - line[i].y, line[i - 1].x - line[i].x);
						Sin = Math.sin(Angle) * overstep;
						Cos = Math.cos(Angle) * overstep;
						LEDpositions.push({
							x: line[i].x + Cos,
							y: line[i].y + Sin
						})
						segmentCounter++
						i = i - 1
					}
				}
			}
		}
		LEDpositions.push({
			x: line[line.length - 1].x,
			y: line[line.length - 1].y
		})
	} else {
		lineDesc.isLine = false
		lineDesc.isDot = true
	}
	lineDesc.LEDpositions = JSON.parse(JSON.stringify(LEDpositions))
}

function rotatePoint(pointx, pointy, pivotx, pivoty, radians) {
	var cosTheta = Math.cos(radians);
	var sinTheta = Math.sin(radians);
	var x = (cosTheta * (pointx - pivotx) - sinTheta * (pointy - pivoty) + pivotx);
	var y = (sinTheta * (pointx - pivotx) + cosTheta * (pointy - pivoty) + pivoty);
	return {
		x: x,
		y: y
	}
}

function scalePoint(pointx, pointy, pivotx, pivoty, scaleFactor) {
	var offsetx = ((pointx - pivotx) * scaleFactor) + pivotx;
	var offsety = ((pointy - pivoty) * scaleFactor) + pivoty;
	return {
		x: offsetx,
		y: offsety
	}
}

function scaleUpDown(scaleFactor) {
	for (i = 0; i < selectedLine.line.length; i++) {
		selectedLine.line[i] = scalePoint(selectedLine.line[i].x, selectedLine.line[i].y, selectedLine.extent.centerX + selectedLine.extent.minX, selectedLine.extent.centerY + selectedLine.extent.minY, scaleFactor)
	}
	updateExtent()
	segment(selectedLine)
	updated = true
}

function checkOverlap(arr) {
	for (let i = 0; i < arr.length; i++) {
		if(!arr[i].hidden){
			for (let j = i + 1; j < arr.length; j++) {
				if(!arr[j].hidden){
					let range1Start = arr[i].startIndex;
					let range1End = arr[i].startIndex + arr[i].numLEDs - 1;
					let range2Start = arr[j].startIndex;
					let range2End = arr[j].startIndex + arr[j].numLEDs - 1;
					if (range1End >= range2Start && range1Start <= range2End) {
						return true;
					}
				}	
			}
		}
	}
	return false;
}

function calculateDistance(x1, y1, p) {
	return Math.hypot(p.x - x1, p.y - y1);
}

function redraw(lineDesc) {

	if(lineDesc.hidden){
		return
	}
	var line = lineDesc.line;
	if(line === undefined){
		return
	}
	var len = line.length;
	var i;
	if(lineDesc.line.length > 1){
		ctx.beginPath();
		ctx.strokeStyle = (lineDesc.done && lineDesc != selectedLine) ? "green" : "red"
		ctx.lineWidth = 14;
		ctx.shadowColor = "rgba(0,0,0,.4)";
		ctx.shadowBlur = 4;
		ctx.shadowOffsetX = 3;
		ctx.shadowOffsetX = 3;
		ctx.lineJoin = "round";
		ctx.lineCap = "round";
		ctx.moveTo(line[0].x, line[0].y);
		for (i = 1; i < line.length; i++) {
			ctx.lineTo(line[i].x, line[i].y);
		}
		ctx.stroke();
	} else {
		ctx.beginPath();
		ctx.lineWidth = 1;
		ctx.arc(line[0].x+0.5, line[0].y+0.5, 7, 0, 2 * Math.PI, false);
		ctx.fillStyle = (lineDesc.done && lineDesc != selectedLine) ? "green" : "red"
		ctx.strokeStyle = (lineDesc.done && lineDesc != selectedLine) ? "green" : "red"
		ctx.fill();
		ctx.stroke();
	}
	
	if (lineDesc == selectedLine) {
		var w = Math.ceil(6);
		ctx.lineWidth = 1;
		ctx.strokeStyle = "black";
		ctx.strokeRect(lineDesc.extent.minX - w, lineDesc.extent.minY - w, lineDesc.extent.width + w * 2, lineDesc.extent.height + w * 2)
	}

	ctx.shadowBlur = 0;
	ctx.shadowOffsetX = 0;
	ctx.shadowOffsetX = 0;
	
	if (lineDesc.LEDpositions.length > 0) {
		for (i = 1; i < lineDesc.LEDpositions.length - 1; i++) {
			ctx.fillStyle = "hsl(0 0% 0%)";
			ctx.fillRect(lineDesc.LEDpositions[i].x - 2, lineDesc.LEDpositions[i].y - 2, 5, 5);
		}
		//1st LED start
		ctx.fillStyle = "hsl(0 0% 0%)";
		ctx.fillRect(lineDesc.LEDpositions[0].x - 4, lineDesc.LEDpositions[0].y - 4, 9, 9);
		ctx.fillStyle = "hsl(0 100% 100%)"
		ctx.fillRect(lineDesc.LEDpositions[0].x - 2, lineDesc.LEDpositions[0].y - 2, 5, 5);
		//1st LED end
		
		ctx.font = "10px Consolas";
		ctx.fillStyle = "#ddd";
		ctx.fillRect(line[0].x+8, line[0].y-6, 3 + (''+lineDesc.startIndex).length*5, 11);
		ctx.fillStyle = "black";
		ctx.fillText(lineDesc.startIndex, line[0].x+9, line[0].y+3); 
		
		if(lineDesc.LEDpositions.length > 1){
			
			//last LED start
			ctx.fillStyle = "hsl(0 0% 0%)";
			ctx.fillRect(lineDesc.LEDpositions[lineDesc.LEDpositions.length - 1].x - 3, lineDesc.LEDpositions[lineDesc.LEDpositions.length - 1].y - 3, 7, 7);
			//last LED end
			
			ctx.font = "10px Consolas";
			ctx.fillStyle = "#ddd";
			ctx.fillRect(line[line.length -1].x+8, line[line.length -1].y-6, 3 + (''+(lineDesc.startIndex + lineDesc.numLEDs -1)).length*5, 11);
			ctx.fillStyle = "black";
			ctx.fillText(lineDesc.startIndex + lineDesc.numLEDs-1, line[line.length -1].x+9, line[line.length -1].y+3); 
		}

		var x = 10;
		var y = 20;
		var lineheight = 15;
		var lines = errstring.split('<br>');
		ctx.font = "15px Verdana";
		ctx.fillStyle = "red";
		for (var i = 0; i<lines.length; i++){
			ctx.fillText(lines[i], x, y + (i*lineheight) );
		}
		var y = 40;
		x = canvas.width - 167
		ctx.fillText(coords, x, y);
	}
	
	if(lineDesc.line.length > 1){
		for (i = 0; i < line.length -1; i++) {
			ctx.fillStyle = "#fff";
			ctx.fillRect(line[i].x - 1, line[i].y - 1, 3, 3);
		}
	}
	
	if(!mouseIsDown){
		if(selectedLine !== undefined ){
			arr = 	[
						selectedLine.extent.minX, 
						selectedLine.extent.minY, 
						canvas.width - selectedLine.extent.maxX, 
						canvas.height - selectedLine.extent.maxY
					]
			
			//console.log(arr[0], arr[1],arr[2], arr[3])
			
			if (arr[2] > 180){ //right
				oleft = selectedLine.extent.maxX + 30
			} else { //left
				oleft = selectedLine.extent.minX - 146
			} 
			if (arr[3] < 145){
				otop = canvas.height
			} else {
				otop = selectedLine.extent.minY + rect.y - 15
			}
			
			jQuery("#props").css("left", oleft)
			jQuery("#props").css("top", otop)
		}
	}
	if(selectedLine !== undefined && !dragging /* && dragged == false */){
		jQuery("#props").show()
		console.log("show")
	} else {
		jQuery("#props").hide()
		//dragged = false
		console.log("hide")
	}
}

function findLineAt(x, y) {
	//TODO: Add initial bounding box check to speed up calculations
	var minLine;
	var distG;
	lineArray.each(function(line) {
		var distL = Infinity
		if(!line.hidden){
			if(line.line !== undefined){
				if(line.line.length > 1){
					for(i = 0; i < line.line.length - 1; i++){
						distL = Math.min(distL, calculateDistance(mouse.x, mouse.y, helpers.closestPointOnLine(mouse.x, mouse.y, line.line[i].x, line.line[i].y, line.line[i+1].x, line.line[i+1].y)))
						//c1 = helpers.getDistToPath(line.line, x, y).c1;
						//c2 = helpers.getDistToPath(line.line, x, y).c2;
						//console.log(helpers.closestPointOnLine(mouse.x, mouse.y, line.line[i].x, line.line[i].y, line.line[i+1].x, line.line[i+1].y))
					}
				} else {
					distL = Math.min(distL, calculateDistance(mouse.x, mouse.y, line.line[0]))
				}
			}
		}
		if(distL < 20){
			if(distG === undefined || distL < distG){
				distG = distL
				minLine = line
			}
		}
	})
	return minLine
}

function getLineMidpoint(vertices) {
	let totalX = 0;
	let totalY = 0;

	for (let i = 0; i < vertices.length; i++) {
		totalX += vertices[i].x;
		totalY += vertices[i].y;
	}

	const midpointX = totalX / vertices.length;
	const midpointY = totalY / vertices.length;

	return { x: midpointX, y: midpointY };
}

function refreshLine(line) {
	if (!line.isLine && !line.isDot) {
		var newLine = {};
		newLine.pivotPoint = {x:0, y:0},
		newLine.gridPoint = grid.checked
		newLine.isLine = false;
		newLine.isDot = false;
		newLine.line = line;
		newLine.id = helpers.getID();
		newLine.startIndex = parseInt(newsindex.value)
		newLine.numLEDs = parseInt(newnleds.value) || 0;
		newLine.LEDpositions = [];
		newLine.done = !dotmode.checked
		if(autoid.checked && !dotmode.checked){
			if(newLine.line.length == 1){
				newsindex.value = (parseInt(newsindex.value) + 1)
			} else {
				newsindex.value = (parseInt(newsindex.value) + parseInt(newnleds.value))
			}
		}
		newLine.hidden = false
		//line.each(extent);
		lineArray.each(function(p){
			p.done = true
		});
	} else {
		var newLine = line;
		//line.line.each(extent);
	}
	//newLine.extent = helpers.copyOfExtent(newLine);
	updateExtent(newLine)
	return newLine;
}

function loadProps(line) {
	if(line !== undefined && line.isDot){
		jQuery("#rotateRight").attr("disabled", true)
		jQuery("#rotateLeft").attr("disabled", true)
		jQuery("#scaleUp").attr("disabled", true)
		jQuery("#scaleDown").attr("disabled", true)
		jQuery("#flip").attr("disabled", true)
		jQuery("#nleds").attr("disabled", true)
		jQuery("#nleds").val(1)
	}else{
		if(line !== undefined){
			jQuery("#nleds").val(line.numLEDs)
		}
		jQuery("#rotateRight").attr("disabled", false)
		jQuery("#rotateLeft").attr("disabled", false)
		jQuery("#scaleUp").attr("disabled", false)
		jQuery("#scaleDown").attr("disabled", false)
		jQuery("#flip").attr("disabled", false)
		jQuery("#nleds").attr("disabled", false)
	}
	if(line !== undefined){
		jQuery("#sindex").val(line.startIndex)
	}
}

function saveProps() {
	if(selectedLine.isDot){
		selectedLine.numLEDs = 1
	}
	else {
		if(isNaN(parseInt(nleds.value))){
			nleds.value = selectedLine.numLEDs
		}
		if(parseInt(nleds.value) > 1){
			selectedLine.numLEDs = parseInt(nleds.value)
		}
	}
	selectedLine.startIndex = !isNaN(parseInt(sindex.value)) && parseInt(sindex.value) > -1 ? parseInt(sindex.value) : 0
	segment(selectedLine)
	updated = true
}

function findDotAt(x, y) {
	var dotidx
	lineArray.each(function(line) {
		if(!line.hidden){
			if(line.line !== undefined){
				line.line.each(function(dot, i) {
					var dista = Math.hypot(dot.x - x, dot.y - y);
					if(dista < 7){
						dotidx = i
					}
				})
			}
		}
	})
	return dotidx
}

function splitLine(c1, c2){
	
	//console.log(c1, c2)
	//console.log(findLineAt(mouse.x, mouse.y))
	//console.log(selectedLine)
	
	console.log(selectedLine.line[c1])
	console.log(selectedLine.line[c2])
	
	
	if(typeof selectedLine.line[c1] != "undefined" && typeof selectedLine.line[c2] != "undefined"){
		offsettox = selectedLine.line[c1].x - selectedLine.line[c2].x
		offsettoy = selectedLine.line[c1].y - selectedLine.line[c2].y
		selectedLine.line.splice(c1+1,0,{x:selectedLine.line[c1].x - offsettox/2,y:selectedLine.line[c1].y - offsettoy/2})
	}
}

function dissolveDot(dot){ 
	if(selectedLine.line.length == 1){
		deleteLine()
		return
	}
	selectedLine.line.splice(dot,1)
}

function drawGrid(){
	
	ctx.beginPath();
	ctx.strokeStyle = grid.checked ? "rgba(0,0,0,.85)" : "rgba(0,0,0,.25)";
	ctx.lineWidth = .5

	if(grid.checked){
		for (var i = 1; i < parseInt(canvwinput.value); i++) {
			ctx.moveTo(Math.round(i*canvas.width/parseInt(canvwinput.value)), 0);
			ctx.lineTo(Math.round(i*canvas.width/parseInt(canvwinput.value)), canvas.height);
		}
		for (var i = 1; i < parseInt(canvhinput.value); i++) {
			ctx.moveTo(0,    Math.round(i*canvas.height/parseInt(canvhinput.value)));
			ctx.lineTo(canvas.width, Math.round(i*canvas.height/parseInt(canvhinput.value)));
		}
	} else {
		for (var i = 1; i < canvas.width/25; i++) {
			ctx.moveTo(Math.round(i*25), 0);
			ctx.lineTo(Math.round(i*25), 10000);
		}
		for (var i = 1; i < canvas.height/25; i++) {
			ctx.moveTo(0,     Math.round(i*25));
			ctx.lineTo(10000, Math.round(i*25));
		}
	}
	ctx.stroke();
}

cntr = 0
msold = 0










function distanceToSegment(point, segA, segB) {
    const x = point.x, y = point.y;
    const x1 = segA.x, y1 = segA.y;
    const x2 = segB.x, y2 = segB.y;
    
    const dx = x2 - x1;
    const dy = y2 - y1;
    
    // If the segment is a point
    if (dx === 0 && dy === 0) {
        return Math.hypot(x - x1, y - y1);
    }

    // Project point onto the line segment, clamping t to [0,1]
    const t = Math.max(0, Math.min(1, ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy)));
    const closestX = x1 + t * dx;
    const closestY = y1 + t * dy;
    
    // Return the distance from the point to the closest point on the segment
    return Math.hypot(x - closestX, y - closestY);
}

function closestSegment(points, cursor) {
    if (points.length < 2) {
		return
        //throw new Error("At least two points are required to form a line segment.");
    }

    let minDistance = Infinity;
    let closestSegmentIndices = null;

    for (let i = 0; i < points.length - 1; i++) {
        const segA = points[i];
        const segB = points[i + 1];
        const distance = distanceToSegment(cursor, segA, segB);

        if (distance < minDistance) {
            minDistance = distance;
            closestSegmentIndices = [i, i + 1];
        }
    }

    return closestSegmentIndices;
}






var dragged = false


function update(pppp, xxxx, yyyy, imp) {
	var imp1 = false
	//var imp2 = false
	if(typeof xxxx != "undefined" && typeof yyyy != "undefined" && imp == 1){
		imp1 = true
		mouse.x = xxxx
		mouse.y = yyyy
	}
	//if(typeof xxxx == "undefined" && typeof yyyy == "undefined" && imp == 2){
	//	imp2 = true
	//}
	cur = "crosshair"

	if(!dragging){
		el = undefined
	}
	if(overCanvas){
		if((findLineAt(mouse.x, mouse.y) !== undefined && selectedLine === undefined || 
			findLineAt(mouse.x, mouse.y) !== undefined && selectedLine == findLineAt(mouse.x, mouse.y)) && firstdot){
			el = findLineAt(mouse.x, mouse.y)
			cur = "move"
			if(findDotAt(mouse.x, mouse.y) !== undefined && selectedLine == findLineAt(mouse.x, mouse.y)){
				el = findDotAt(mouse.x, mouse.y)
				cur = grid.checked ? "move" : "pointer"
				coords = "Dot x: " + Math.round(selectedLine.line[el].x * scale) + " y: " + Math.round(selectedLine.line[el].y * scale)
				offsetmx = mouse.x - selectedLine.line[el].x
				offsetmy = mouse.y - selectedLine.line[el].y
				updated = true
			}
		}
		setCursor(cur)
	}
	
	if((drawMode == "Draw" && el !== undefined && !mouse.lastButton && mouse.button && mouse.which[0]) || selectedLine !== undefined){
		//console.log(1)
		drawMode = "Edit"
		firstdot = true
	} else if(selectedLine === undefined){
		//console.log(2)
		drawMode = "Draw"
	}
	
	noGrid = false
	
	if(selectedLine !== undefined){
		if(selectedLine.isLine){
			noGrid = true
		}
	}
	
	if(grid.checked && !noGrid){
		if (!mouse.lastButton && mouse.button){
			if(el !== undefined && !mouse.lastButton){
				dragging = true
				dragOffX = mouse.x;
				dragOffY = mouse.y;
				if(el != 0){
					console.log(1)
					selectedLine = el
					loadProps(selectedLine)
				}
			} 
		} else if (mouse.lastButton && mouse.button){
			if(selectedLine === undefined){
				xxm = mouse.x
				yym = mouse.y
				
				xx = Math.floor(xxm/(canvas.width/ parseInt(canvwinput.value)))
				yy = Math.floor(yym/(canvas.height/parseInt(canvhinput.value)))
				
				xxcenter = Math.floor(xxm/(canvas.width/ parseInt(canvwinput.value))) * (canvas.width/ parseInt(canvwinput.value)) + (canvas.width/ parseInt(canvwinput.value)/2)
				yycenter = Math.floor(yym/(canvas.height/parseInt(canvhinput.value))) * (canvas.height/parseInt(canvhinput.value)) + (canvas.height/parseInt(canvhinput.value)/2)
				
				var dista = Math.hypot(xxcenter - xxm, yycenter - yym);
				if(dista < canvas.width/ parseInt(canvwinput.value)*.75/2){
					flag = false
					lineArray.each(function(p){
						if(p.line[0].x == xxcenter && p.line[0].y == yycenter){
							flag = true
						}
					})
					
					if(!flag){
						points = [];
						lineArray.push(points);
						//console.log("p1")
						points.push({
							x: xxcenter,
							y: yycenter,
						});
						points = refreshLine(lineArray.pop())
						lineArray.push(points)
						simplify(points)
						segment(points)
						updateExtent(points)
					}
				}
			} else {
				if(dragging){
					var ox = mouse.x - dragOffX;
					var oy = mouse.y - dragOffY;
					
					helpers.resetExtent();
					selectedLine.line.each(function(p) {
						p.x += ox;
						p.y += oy;
						helpers.extent(p);
						return p;
					})
					selectedLine.extent = helpers.copyOfExtent(selectedLine);
					segment(selectedLine)
					dragOffX = mouse.x;
					dragOffY = mouse.y;
				} else {

				}
			}
		} else if (mouse.lastButton && !mouse.button){
			if(selectedLine !== undefined && findLineAt(mouse.x, mouse.y) != selectedLine){
				selectedLine = undefined
			} else {
				
			}
		}
		
		if (mouse.which[0]) {
			
		} else {
			if(dragging){
				if(selectedLine !== undefined){
					dragging = false
					xxm = mouse.x - 2
					yym = mouse.y - 1
					xxcenter = Math.floor(xxm/(canvas.width/ parseInt(canvwinput.value))) * (canvas.width/ parseInt(canvwinput.value)) + (canvas.width/ parseInt(canvwinput.value)/2)
					yycenter = Math.floor(yym/(canvas.height/parseInt(canvhinput.value))) * (canvas.height/parseInt(canvhinput.value)) + (canvas.height/parseInt(canvhinput.value)/2)
					selectedLine.line[0].x = xxcenter
					selectedLine.line[0].y = yycenter
					updateExtent()
					segment(selectedLine)
				}
			}
		}
	} else {
		
		/*
		if(mouse.button){
			cntr++
		}
		if(cntr < 8){
			return
		} else {
			cntr = 0
		}		
		*/
		
		if (drawMode == "Draw") {
			if ((!mouse.lastButton && mouse.button) || imp1) {
				if(mouse.isTouch){
					cntr = 1
				}
				if (mouse.which[0] || imp1) {
					points = [];
					drawing = true;
					lineArray.push(points);
					if(dotmode.checked && firstdot == false){
						//console.log("p2")
						points.push({
							x: lineArray[helpers.currentID()-1].line[lineArray[helpers.currentID()-1].line.length-1].x,
							y: lineArray[helpers.currentID()-1].line[lineArray[helpers.currentID()-1].line.length-1].y,
						});
					} else {
						if(fortyfive && dotmode.checked){
							//console.log("p4")
							points.push({
								x: pointOnGuide.x-2,
								y: pointOnGuide.y-2,
							});
						} else {
							//console.log("p5")
							if(imp1){
								points.push({
									x: xxxx - 2,
									y: yyyy - 2
								});
							}
							points.push({
								x: mouse.x - 2,
								y: mouse.y - 2
							});
						}
					}
				}
			} else {
				if (drawing && mouse.button) {
					if(mouse.isTouch){
						cntr++;
						if(pointer.count == 2){
							//TODO
							lineArray.pop()
							endDrawingAndDeselct()
							requestAnimationFrame(update)
							drawing = false
							return
						}
					};
					if(fortyfive && dotmode.checked){
						//console.log("p4")
						points.push({
							x: pointOnGuide.x-2,
							y: pointOnGuide.y-2,
						});
					} else {
						//console.log("p5")
						points.push({
							x: mouse.x - 2,
							y: mouse.y - 2
						});
					}
					if(mouse.isTouch && cntr > touchDelay || !mouse.isTouch){
						var p1 = points[points.length - 2];
						var p2 = points[points.length - 1];
						ctx.lineWidth = 1;
						ctx.strokeStyle = "black";
						ctx.beginPath();
						ctx.moveTo(p1.x, p1.y);
						ctx.lineTo(p2.x, p2.y);
						ctx.stroke();
					} else {}
				} else {
					if (drawing) {
						if(mouse.isTouch && cntr < touchDelay){
							points = [];
							lineArray.pop()
							requestAnimationFrame(update)
							drawing = false
							return
						}
						if(fortyfive && dotmode.checked){
							//console.log("p6")
							points.push({
								x: pointOnGuide.x-2,
								y: pointOnGuide.y-2,
							});
						} else {
							//console.log("p7")
							points.push({
								x: mouse.x - 2,
								y: mouse.y - 2
							});
						}
						clean(points)
						if(dotmode.checked){
							if(firstdot){
								points = refreshLine(lineArray.pop())
								lineArray.push(points)
								firstdot = false
							} else {
								lineArray.each(function(p) {
									if(p.id == helpers.currentID()){
										p.line = p.line.concat(points);
									}
								})
								lineArray.pop()
								points = refreshLine(lineArray[helpers.currentID()-1])
							}
						}else{
							points = refreshLine(lineArray.pop())
							lineArray.push(points)
						}
						simplify(points)
						segment(points)
						updateExtent(points)
						drawing = false;
					}
				}
			}
		} else if (drawMode == "Edit" ) {
			if (dragging || mouse.button) {
				if(mouse.button && !dragging && mouse.which[0]){
					if(findDotAt(mouse.x, mouse.y) !== undefined && selectedLine == findLineAt(mouse.x, mouse.y)){
						dragDots = true
					} else {
						dragDots = false
					}
				}
				if(dragDots){
					if (!mouse.lastButton && mouse.button){
						if(mouse.which[0]) {
							if(findLineAt(mouse.x, mouse.y) !== undefined && selectedLine == findLineAt(mouse.x, mouse.y)){
								selectedLine = findLineAt(mouse.x, mouse.y)
								selectedDot = findDotAt(mouse.x, mouse.y)
								console.log(2)
								loadProps(selectedLine)
							}
						}
					}
					if (selectedDot !== undefined && !mouse.lastButton && mouse.button) {
						dragging = true;
						jQuery("#props").hide()
						omx = offsetmx
						omy = offsetmy
					} else if(selectedDot !== undefined){
						jQuery("#props").hide()
						if (dragging && !mouse.button) {
							dragging = false;
							selectedLine.line[selectedDot].x = mouse.x - omx
							selectedLine.line[selectedDot].y = mouse.y - omy
							segment(selectedLine)
							selectedDot = undefined
						} 
						if (dragging && mouse.button) {
							selectedLine.line[selectedDot].x = mouse.x - omx
							selectedLine.line[selectedDot].y = mouse.y - omy
							updateExtent()
							segment(selectedLine)
						}
					} else {
						selectedLine = undefined
						selectedDot = undefined
						loadProps()
					}
				}else{
					if (selectedLine === undefined && !mouse.lastButton && mouse.button){
						if(mouse.which[0]) {
							if(findLineAt(mouse.x, mouse.y) !== undefined){
								dragged = false;
								selectedLine = findLineAt(mouse.x, mouse.y)
								console.log(3)
								loadProps(selectedLine)
							}
						}
					} else if (selectedLine !== undefined && !mouse.lastButton && mouse.button){
						if(mouse.which[0]) {
							if(findLineAt(mouse.x, mouse.y) === undefined || findLineAt(mouse.x, mouse.y) !== selectedLine){
								selectedLine = undefined
								selectedDot = undefined
								loadProps()
							}
						} else if(mouse.which[2]){
							var dot = findDotAt(mouse.x, mouse.y)
							if(dot !== undefined || selectedLine.line.length == 1){
								dissolveDot(dot)
							} else {
								
								//console.log(findLineAt(mouse.x, mouse.y).line)
								//console.log([mouse.x, mouse.y])
								//console.log(closestSegment(findLineAt(mouse.x, mouse.y).line, {x: mouse.x,y: mouse.y}))
								if(typeof findLineAt(mouse.x, mouse.y) != "undefined"){
									var seg = closestSegment(findLineAt(mouse.x, mouse.y).line, {x: mouse.x,y: mouse.y})
									splitLine(seg[0], seg[1]);
								}
							}
						}
					}
					jQuery("#props").hide()
					if (selectedLine !== undefined && !mouse.lastButton && mouse.button) {
						dragging = true;
						dragOffX = mouse.x;
						dragOffY = mouse.y;
						
					} else {
						if (dragging && !mouse.button) {
							dragging = false;
							if(dragged){
								selectedLine = undefined
							}
						} else if (dragging) {
							var ox = mouse.x - dragOffX;
							var oy = mouse.y - dragOffY;
							if(ox != 0 || oy != 0){
								dragged = true;
							}
							helpers.resetExtent();
							selectedLine.line.each(function(p) {
								p.x += ox;
								p.y += oy;
								helpers.extent(p);
								return p;
							})
							selectedLine.extent = helpers.copyOfExtent(selectedLine);
							segment(selectedLine)
							dragOffX = mouse.x;
							dragOffY = mouse.y;
						}
					}
				}
			}else{
				dragDots = false
			}
		}
	}
	mouse.lastButton = mouse.button;
	if(updated){
		redrawAll()
		updated = false
		exportMatrix()
	}
	console.log(dragged)
	requestAnimationFrame(update);
}

requestAnimationFrame(update);