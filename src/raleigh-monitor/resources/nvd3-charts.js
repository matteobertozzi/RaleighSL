_charts = {}

function setupChart(canvasId, chart) {
    d3.select('#' + canvasId + ' svg')
      //.datum([])
      .transition().duration(0)
      .call(chart);
    nv.utils.windowResize(chart.update);
    _charts[canvasId] = chart;
    return chart;
}

function StackedAreaChart(canvasId) {
  nv.addGraph(function() {
    var chart = nv.models.stackedAreaChart()
                  .x(function(d) { return d[0] })
                  .y(function(d) { return d[1] })
                  .clipEdge(true);

    chart.xAxis
        .showMaxMin(false)
        .tickFormat(function(d) { return d3.time.format('%X')(new Date(d)) });

    chart.yAxis
        .tickFormat(d3.format(',.2f'));

    return setupChart(canvasId, chart);
  });
}

function CumulativeLineChart(canvasId) {
  nv.addGraph(function() {
    var chart = nv.models.cumulativeLineChart()
                  .x(function(d) { return d[0] })
                  .y(function(d) { return d[1] })
                  //.x(function(d) { return d[0] })
                  //.y(function(d) { return d[1]/100 }) //adjusting, 100% is 1.00, not 100 as it is in the data
                  //.color(d3.scale.category10().range());

    chart.xAxis
        .tickFormat(function(d) {
          return d3.time.format('%X')(new Date(d))
        });

    chart.yAxis
        .tickFormat(d3.format(',.2f'));
        //.tickFormat(d3.format(',.1%'));

    return setupChart(canvasId, chart);
  });
}

function MultiBarChart(canvasId) {
  nv.addGraph(function() {
    var chart = nv.models.multiBarChart();

    chart.xAxis
        .tickFormat(d3.format(',f'));

    chart.yAxis
        .tickFormat(d3.format(',.1f'));

    return setupChart(canvasId, chart);
  });
}

function PieChart(canvasId) {
  nv.addGraph(function() {
    var chart = nv.models.pieChart()
      .x(function(d) { return d.label })
      .y(function(d) { return d.value })
      .showLabels(true)
      .labelThreshold(.05)
      .donut(true);

    return setupChart(canvasId, chart);
  });
}

function updateChart(canvasId, dataa) {
  chart = _charts[canvasId];
  x = d3.select('#' + canvasId +' svg')
      .datum(dataa)
      .transition().duration(100);
  x.call(chart.update);
}
