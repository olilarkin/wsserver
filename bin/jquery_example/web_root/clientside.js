var gConnection; // websocket gConnection

const kDisplayRefreshRate = 25;
const kStatusRefreshRate = 5;

const kPBStopped = 0;
const kPBPlaying = 1;
const kPBPaused = 2;

function filePlayerStatusObj()
{
	this.h = 0;
	this.m = 0;
	this.s = 0;
	this.pbstate = kPBStopped;
	this.loop = 0;
	this.filename = "no file loaded";
	this.msg = "";
	this.file = -1;
}

function statusObj() {
	this.files = [];
	this.gain = 0;
	this.fp = new filePlayerStatusObj();
	this.meterl = 0;
	this.meterr = 0;
}

//global variables
var gStatus; // instance of statusObj containing representation of current status
var gRefreshIDDisplay;
var gPrevState;

$(document).ready(function() {
	gStatus = new statusObj;
	gRefreshIDDisplay = setInterval("updateDisplay()", kDisplayRefreshRate);
	
	updateFileList();
	
  ws_connect();
});

function updateFileList()
{
	$("#FileList").html("");
	
	if(gStatus.files.length == 0) {
		$("#FileList").append("<option value='" + 0 + "'>No Files Found</option>");
		$("#FileList").attr("disabled", true); 
	}
	else
	{
		$("#FileList").removeAttr("disabled");
		$("#FileList").append("<option value='0'>Choose file...</option>");

		for (i=1;i<=gStatus.files.length;i++)
			$("#FileList").append("<option value='" + i + "'>" + gStatus.files[i-1]  + "</option>");
	}
}

function updateDisplay() {
	if(gStatus.gain != $( "#slider" ).val) {
		$("#slider").slider( "option", "value", gStatus.gain );
		$("#amount").val(  gStatus.volume );
	}

	$("#FilePlayerStatusDisplay").html( "<p>" + gStatus.fp.filename + ":" + gStatus.fp.msg + ", " +
									gStatus.fp.h + ":" +
									gStatus.fp.m + ":" +
									gStatus.fp.s + "</p>");
	
	if( gPrevState != gStatus.fp.pbstate) {
	
		if(gStatus.fp.pbstate == kPBPlaying) {
			$( "#play" ).button( "option", {
				label: "pause",
				icons: {
					primary: "ui-icon-pause"
				}
			});
		}
		else if(gStatus.fp.pbstate == kPBStopped) {
				$( "#play" ).button( "option", {
				label: "play",
				icons: {
					primary: "ui-icon-play"
				}
			});
		}
		
		gPrevState = gStatus.fp.pbstate;
	}
	
	if(gStatus.files.length != $('#FileList option').size()-1)
		updateFileList();
	
	$("#MeterLeft").progressbar( "option", "value", gStatus.meterl);
	$("#MeterRight").progressbar( "option", "value", gStatus.meterr);
}

function sendStatusToServer() {
	gConnection.send("updateStatusFromClient " +  JSON.stringify(gStatus) );
}

$(function() {

	//Volume Slider
	$( "#slider" ).slider({
		orientation: "horizontal",
		range: "min",
		min: -70,
		max: 12,
		value: 0,
		slide: function( event, ui ) {
			gStatus.gain = ui.value;
			gConnection.send("gain " +  gStatus.gain );
			$( "#readout" ).val(  gStatus.gain );
		}
	});
	
	//Volume DB Readout
	$( "#readout" ).val( $( "#slider" ).slider( "value" ) );
	
	//Meter
	$("#MeterLeft").progressbar({ value: 0 });
	$("#MeterRight").progressbar({ value: 0 });

	//List of files
	
	$("#FileList").change(function() {  
		var newFile = $("#FileList").val() - 1;
		gConnection.send("file " + newFile);
		$("#FileList").val(0)
	});
	
	//File Player Playback Controls
	
	$( "#beginning" ).button({
		text: false,
		icons: {
			primary: "ui-icon-seek-start"
		}
	})
	.click(function() {
		gConnection.send("pbbutton prev");
	});
	
	$( "#rewind" ).button({
		text: false,
		icons: {
			primary: "ui-icon-seek-prev"
		}
	})
	.click(function() {
		gConnection.send("pbbutton rewind");
	});
	
	$( "#play" ).button({
		text: false,
		icons: {
			primary: "ui-icon-play"
		}
	})
	.click(function() {		
		if(gStatus.files.length && gStatus.fp.file != -1) {
			if ( $( this ).text() === "play" ) {
				gConnection.send("pbbutton play");
			} else {
				gConnection.send("pbbutton pause");
			}
		}
	});
	
	$( "#stop" ).button({
		text: false,
		icons: {
			primary: "ui-icon-stop"
		}
	})
	.click(function() {
		if(gStatus.files.length && gStatus.fp.file != -1) {
			gConnection.send("pbbutton stop");
		}
	});
	
	$( "#forward" ).button({
		text: false,
		icons: {
			primary: "ui-icon-seek-next"
		}
	})
	.click(function() {
		gConnection.send("pbbutton forward");
	});
	
	$( "#end" ).button({
		text: false,
		icons: {
			primary: "ui-icon-seek-end"
		}
	})
	.click(function() {
		gConnection.send("pbbutton next");
	});
	
	$( "#loop" ).button().click(function() {
		gConnection.send("loop " +  +!$( this ).is(':checked') );
	});
});

function ws_connect() {
    if ('WebSocket' in window) {

        console.log('Connecting');
        gConnection = new WebSocket('ws://' + window.location.host + '/maxmsp');
        gConnection.onopen = function(ev) {
        
            //connectButton.label = "WebSocket Disconnect";
            console.log('CONNECTED');
            var message = 'update on';
            console.log('SENT: ' + message);
            gConnection.send(message);
        };

        gConnection.onclose = function(ev) {
            //connectButton.label = "WebSocket Connect";
            console.log('DISCONNECTED');
        };

        gConnection.onmessage = function(ev) {
          //TODO: handle messages
          if(ev.data.substr(0, 3) == "rx ")
          {
            json = ev.data.substr(3);
            
            gStatus = jQuery.parseJSON(json);
          }
          
          console.log('RECEIVED: ' + ev.data);
        };

        gConnection.onerror = function(ev) {
            alert("WebSocket error");
        };

    } else {
        alert("WebSocket is not available!!!\n" +
              "Demo will not function.");
    }
}