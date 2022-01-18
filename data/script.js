// Get current sensor readings when the page loads  
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', initAll());


function initAll(){
  initWebSocket();
  getReadings();
}
// Function to get current readings on the webpage when it loads for the first time
function getReadings(){  
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var working = myObj.working;
      var tempONOFF = myObj.temperature;
      var tempAmbient = myObj.ambient;
      if (working == 0){
        document.getElementById('1').innerHTML= "Encender";
      }else{
        document.getElementById('1').innerHTML= "Apagar";
      }      
      document.getElementById('tempON').innerHTML= tempONOFF;
      document.getElementById('tempAmb').innerHTML= tempAmbient;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

function changeParameter(element){
  console.log(element.id);
  console.log("pene");
  websocket.send(element.id);
}

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
  websocket.send("states");
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
} 

function onMessage(event) {
  console.log(event.data);
  getReadings();
}