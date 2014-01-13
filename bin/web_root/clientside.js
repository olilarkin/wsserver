var gConnection; // websocket gConnection

var panel;
var connectButton;
var label;
var xypad;

function init() {  
   panel = new Interface.Panel({  
    background:"#fff", 
    stroke:"000",
    container:$("#panel"),
    useRelativeSizesAndPositions : true
  }); 
  
  connectButton = new Interface.Button({ 
    background:"#fff",
    bounds:[0.,0.,0.2,0.05 ],  
    label:'WebSocket Connect',    
    size:14,
    stroke:"000",
    style:'normal',
    onvaluechange: function() {
      this.clear();
      toggleConnection();
    }
  });
  
  label = new Interface.Label({ 
    bounds:[0.21,0.,0.9, 0.05],
    value:'',
    hAlign:'left',
    vAlign:'middle',
    size:12,
    stroke:"000",
    style:'normal'
  });
  
  xypad = new Interface.XY({
    background:"#fff",
    stroke:"000",
    childWidth: 40,
    numChildren: 1,
    bounds:[0,0.06,0.9,0.9],
    usePhysics : false,
    friction : 0.9,
    activeTouch : true,
    maxVelocity : 100,
    detectCollisions : true,
    onvaluechange : function() {
      if(gConnection) 
        gConnection.send(JSON.stringify(this.values[0], function(key, val) {
                                              return val.toFixed ? Number(val.toFixed(3)) : val;
                                          })
        );
    },
    oninit: function() { this.rainbow() }
  });
  
  panel.add(connectButton, label, xypad);
}

function writeToScreen (message) {
  label.clear();
  label.setValue(message);
}

function ws_connect() {
    if ('WebSocket' in window) {

        writeToScreen('Connecting');
        gConnection = new WebSocket('ws://' + window.location.host + '/maxmsp');
        gConnection.onopen = function(ev) {
        
            connectButton.label = "WebSocket Disconnect";
//            document.getElementById("update").disabled=false;
//            document.getElementById("update").innerHTML = "Disable Update";
            writeToScreen('CONNECTED');
            var message = 'update on';
            writeToScreen('SENT: ' + message);
            gConnection.send(message);
        };

        gConnection.onclose = function(ev) {
//            document.getElementById("update").disabled=true;
//            document.getElementById("update").innerHTML = "Enable Update";
            connectButton.label = "WebSocket Connect";
            writeToScreen('DISCONNECTED');
        };

        gConnection.onmessage = function(ev) {
          //TODO: handle messages
          if(ev.data.substr(0, 3) == "rx ")
          {
            json = ev.data.substr(3);
            
            if(json.substr(0, 5) == "move ")
            {
              values = JSON.parse(json.substr(5));
              xypad.children[0].x = values.x * xypad._width();
              xypad.children[0].y = values.y * xypad._height();
              //console.log(xypad.children[0]);
              xypad.refresh();
            }
          }
          
          writeToScreen('RECEIVED: ' + ev.data);
        };

        gConnection.onerror = function(ev) {
            alert("WebSocket error");
        };

    } else {
        alert("WebSocket is not available!!!\n" +
              "Demo will not function.");
    }
}

// user connect/disconnect
function toggleConnection() {
    if (connectButton.label == "WebSocket Connect") {
      ws_connect();
    }
    else {
      gConnection.close();
    }
}
//
//// user turn updates on/off
//function toggleUpdate(el) {
//    var tag=el.innerHTML;
//    var message;
//    if (tag == "Enable Update") {
//        message = 'update on';
//        el.innerHTML = "Disable Update";
//    }
//    else {
//        message = 'update off';
//        el.innerHTML = "Enable Update";
//    }
//    writeToScreen('SENT: ' + message);
//    gConnection.send(message);
//}
