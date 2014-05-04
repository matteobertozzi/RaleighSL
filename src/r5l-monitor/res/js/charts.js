function Chart(canvasId, updateInterval, url) {
  $(canvasId).html("Loading...");

  function refreshChart() {
    $.get(url, function(data) {
      $(canvasId).html(data);
    });
  }

  var interval = setInterval(function () {
    refreshChart();
  }, updateInterval);

  $(canvasId)
    .mouseover(function() {
      clearInterval(interval);
    })
    .mouseout(function() {
      interval = setInterval(function () {
        refreshChart();
      }, updateInterval);
    });
}

function BarChart(canvasId, updateInterval, url) {
  Chart(canvasId, updateInterval, url);
}

function LineChart(canvasId, updateInterval, url) {
  Chart(canvasId, updateInterval, url);
}
