// http://www.websocket.org/echo.html
function initMetricsWebSocket(wsUri) {
  websocket = new WebSocket(wsUri);
  websocket.onopen = function(evt) {
    console.log('open ');
  };
  websocket.onclose = function(evt) {
    console.log('close ' + evt.data);
    setTimeout(function() { initMetricsWebSocket(wsUri) }, 5000);
  };
  websocket.onmessage = function(evt) {
    console.log('message ' + evt.data);
  };
  websocket.onerror = function(evt) {
    //console.log('error ' + evt.data);
  };
}