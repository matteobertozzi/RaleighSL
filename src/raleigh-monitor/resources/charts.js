function BarChart(canvasId, updateInterval, url) {
  var margin = {
    top: 20,
    right: 20,
    bottom: 30,
    left: 50
  },
    width = 960 - margin.left - margin.right,
    height = 200 - margin.top - margin.bottom;

  var formatPercent = d3.format(".0%");

  var x = d3.scale.ordinal()
    .rangeRoundBands([0, width], .1);

  var y = d3.scale.linear()
    .range([height, 0]);

  var xAxis = d3.svg.axis()
    .scale(x)
    .orient("bottom");

  var yAxis = d3.svg.axis()
    .scale(y)
    .orient("left")
    .tickFormat(formatPercent);

  /*var tip = d3.tip()
  .attr('class', 'd3-tip')
  .offset([-10, 0])
  .html(function(d) {
    return "<strong>val:</strong> <span style='color:red'>" + d.val + "</span>";
  })*/



  var svg = d3.select(canvasId).append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

  //svg.call(tip);

  d3.json(url, function (error, data) {
    x.domain(data.map(function(d) { return d.key; }));
    y.domain([0, d3.max(data, function(d) { return d.val; })]);






    svg.selectAll(".bar")
      .data(data)
    .enter().append("rect")
      .attr("class", "bar")
      .attr("x", function(d) { return x(d.key); })
      .attr("width", x.rangeBand())
      .attr("y", function(d) { return y(d.val); })
      .attr("height", function(d) { return height - y(d.val); });
      //.on('mouseover', tip.show)
      //.on('mouseout', tip.hide);

    svg.selectAll("text")
              .data(data)
              .enter()
              .append("text")
              .text(function(d) {return d.val;
              })
              .attr("x", function(d, i) { return x(d.key)+10;
              })
              .attr("y", function(d) { return y(d.val);
              })
              .attr("font-family", "sans-serif")
              .attr("font-size", "16px")
              .attr("fill", "yellow");

       svg.append("g")
      .attr("class", "x axis")
      .attr("transform", "translate(0," + height + ")")
      .call(xAxis);

    svg.append("g")
      .attr("class", "y axis")
      .call(yAxis)
      .append("text")
      .attr("transform", "rotate(-90)")
      .attr("y", 6)
      .attr("dy", ".71em")
      .style("text-anchor", "end")
      .text("Value ($)");



    setInterval(function () {
      d3.json(url, function (error, data) {
        x.domain(data.map(function(d) { return d.key; }));
        y.domain([0, d3.max(data, function(d) { return d.val; })]);

        svg.selectAll(".bar")
      .data(data)
      .attr("x", function(d) { return x(d.key); })
      .attr("width", x.rangeBand())
      .attr("y", function(d) { return y(d.val); })
      .attr("height", function(d) { return height - y(d.val); });
      //.on('mouseover', tip.show)
      //.on('mouseout', tip.hide);
       svg.selectAll("text")
              .data(data)
              .text(function(d) {return d.val;
              })
              .attr("x", function(d) { return x(d.key);
              })
              .attr("y", function(d) { return y(d.val);
              })
              .attr("font-family", "sans-serif")
              .attr("font-size", "14px")
              .attr("fill", "red");


       /* svg.select(".x.axis")
          .call(xAxis);
        svg.select(".y.axis")
          .call(yAxis);*/
      });
    }, updateInterval);
  });
}

function AreaChart(canvasId, updateInterval, url) {
  var margin = {
    top: 20,
    right: 20,
    bottom: 30,
    left: 50
  },
    width = 960 - margin.left - margin.right,
    height = 200 - margin.top - margin.bottom;

  var parseDate = d3.time.format("%d-%b-%y-%H:%M:%S").parse;

  var x = d3.time.scale()
    .range([0, width]);

  var y = d3.scale.linear()
    .range([height, 0]);

  var xAxis = d3.svg.axis()
    .scale(x)
    .orient("bottom");

  var yAxis = d3.svg.axis()
    .scale(y)
    .orient("left");

  var area = d3.svg.area()
    .interpolate("basis")
    .x(function (d) {
      return x(d.date);
    })
    .y0(height)
    .y1(function (d) {
      return y(d.close);
    });

   var valueline = d3.svg.line()
      .interpolate("basis")
      .x(function(d) {return x(d.date); })
      .y(function(d) {return y(d.close); });


  var svg = d3.select(canvasId).append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

  /*svg.append("rect")
        .attr("x", 0)
        .attr("y", 0)
        .attr("width", width)
        .attr("height", height)
        .attr("fill", "yellow");*/

  d3.json(url, function (error, data) {
    data.forEach(function (d) {
      d.date = parseDate(d.date);
      d.close = +d.close;
    });

    x.domain(d3.extent(data, function (d) {
      return d.date;
    }));
    y.domain([0, d3.max(data, function (d) {
      return d.close;
    })]);

    svg.append("path")
      .datum(data)
      .attr("class", "area")
      .attr("d", area);

    svg.append("path")
    .attr("class", "line")
    .attr("d", valueline(data));

    svg.append("g")
      .attr("class", "x axis")
      .attr("transform", "translate(0," + height + ")")
      .call(xAxis);

    svg.append("g")
      .attr("class", "y axis")
      .call(yAxis)
      .append("text")
      .attr("transform", "rotate(-90)")
      .attr("y", 6)
      .attr("dy", ".71em")
      .style("text-anchor", "end")
      .text("Value ($)");

    setInterval(function () {
      d3.json(url, function (error, data) {
        data.forEach(function (d) {
          d.date = parseDate(d.date);
          d.close = +d.close;
        });

        x.domain(d3.extent(data, function (d) {
          return d.date;
        }));
        y.domain([0, d3.max(data, function (d) {
          return d.close;
        })]);

        svg.selectAll("path")
          .datum(data)
          .attr("d", area);
        svg.append("path")
          //.attr("class", "line")
          .attr("d", valueline(data));
        svg.select(".x.axis")
          .call(xAxis);
        svg.select(".y.axis")
          .call(yAxis);
      });
    }, updateInterval);
  });
}

function StackedAreaChart(canvasId, updateInterval, url) {
  var margin = {
    top: 20,
    right: 20,
    bottom: 30,
    left: 50
  },
    width = 960 - margin.left - margin.right,
    height = 200 - margin.top - margin.bottom;

  var parseDate = d3.time.format("%d-%b-%y-%H:%M:%S").parse,
    formatPercent = d3.format(".0%");

  var x = d3.time.scale()
    .range([0, width]);

  var y = d3.scale.linear()
    .range([height, 0]);

  var color = d3.scale.category20b();

  var xAxis = d3.svg.axis()
    .scale(x)
    .orient("bottom");

  var yAxis = d3.svg.axis()
    .scale(y)
    .orient("left")
    .tickFormat(formatPercent);

  var area = d3.svg.area()
    .x(function (d) {
      return x(d.date);
    })
    .y0(function (d) {
      return y(d.y0);
    })
    .y1(function (d) {
      return y(d.y0 + d.y);
    });

  var stack = d3.layout.stack()
    .values(function (d) {
      return d.values;
    });

  var svg = d3.select(canvasId).append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

  d3.json(url, function (error, data) {
    color.domain(d3.keys(data[0]).filter(function (key) {
      return key !== "date";
    }))
    data.forEach(function (d) {
      d.date = parseDate(d.date);
    });
    var ditems = stack(color.domain().map(function (name) {
      return {
        name: name,
        values: data.map(function (d) {
          return {
            date: d.date,
            y: d[name] / 100
          };
        })
      };
    }));

    x.domain(d3.extent(data, function (d) {
      return d.date;
    }));

    var ditem = svg.selectAll(".ditem")
      .data(ditems)
      .enter().append("g")
      .attr("class", "ditem");

    ditem.append("path")
      .attr("class", "area")
      .attr("d", function (d) {
        return area(d.values);
      })
      .style("fill", function (d) {
        return color(d.name);
      });

    ditem.append("text")
      .datum(function (d) {
        return {
          name: d.name,
          value: d.values[d.values.length - 1]
        };
      })
      .attr("transform", function (d) {
        return "translate(" + x(d.value.date) + "," + y(d.value.y0 + d.value.y / 2) + ")";
      })
      .attr("x", -6)
      .attr("dy", ".35em")
      .text(function (d) {
        return d.name;
      });

    svg.append("g")
      .attr("class", "x axis")
      .attr("transform", "translate(0," + height + ")")
      .call(xAxis);

    svg.append("g")
      .attr("class", "y axis")
      .call(yAxis);

    setInterval(function () {
      d3.json(url, function (error, data) {
        color.domain(d3.keys(data[0]).filter(function (key) {
          return key !== "date";
        }))
        data.forEach(function (d) {
          d.date = parseDate(d.date);
        });
        var ditems = stack(color.domain().map(function (name) {
          return {
            name: name,
            values: data.map(function (d) {
              return {
                date: d.date,
                y: d[name] / 100
              };
            })
          };
        }));

        x.domain(d3.extent(data, function (d) {
          return d.date;
        }));

        var ditem = svg.selectAll(".ditem")
          .data(ditems);

        ditem.select(".area")
          .attr("d", function (d) { return area(d.values); })
          .style("fill", function (d) { return color(d.name); });

        ditem.select("text")
          .datum(function (d) { return { name: d.name, value: d.values[d.values.length - 1] }; })
          .attr("transform", function (d) { return "translate(" + x(d.value.date) + "," + y(d.value.y0 + d.value.y / 2) + ")"; })
          .text(function (d) { return d.name; });

        svg.select(".x.axis")
          .call(xAxis);
        svg.select(".y.axis")
          .call(yAxis);
      });
    }, updateInterval);
  });
}

function MultilineChart(canvasId, yLabel, updateInterval, url) {
  var margin = {top: 20, right: 80, bottom: 30, left: 80},
    width = 960 - margin.left - margin.right,
    height = 130 - margin.top - margin.bottom;

var parseDate = d3.time.format("%d-%b-%y-%H:%M:%S").parse;

var x = d3.time.scale()
    .range([0, width]);

var y = d3.scale.linear()
    .range([height, 0]);

var color = d3.scale.category10();

var xAxis = d3.svg.axis()
    .scale(x)
    .orient("bottom");

var yAxis = d3.svg.axis()
    .scale(y)
    .ticks(6)
    .orient("left");

var line = d3.svg.line()
    .interpolate("basis")
    .x(function(d) { return x(d.date); })
    .y(function(d) { return y(d.val); });

var svg = d3.select(canvasId).append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
  .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

d3.json(url, function (error, data) {
  color.domain(d3.keys(data[0]).filter(function(key) { return key !== "date"; }));

  data.forEach(function(d) {
    d.date = parseDate(d.date);
  });

  var ditems = color.domain().map(function(name) {
    return {
      name: name,
      values: data.map(function(d) {
        return {date: d.date, val: +d[name]};
      })
    };
  });

  x.domain(d3.extent(data, function(d) { return d.date; }));

  y.domain([
    d3.min(ditems, function(c) { return d3.min(c.values, function(v) { return v.val; }); }),
    d3.max(ditems, function(c) { return d3.max(c.values, function(v) { return v.val; }); })
  ]);

  svg.append("g")
      .attr("class", "x axis")
      .attr("transform", "translate(0," + height + ")")
      .call(xAxis);

  svg.append("g")
      .attr("class", "y axis")
      .call(yAxis)
    .append("text")
      .attr("transform", "rotate(-90)")
      .attr("y", 6)
      .attr("dy", ".75em")
      .style("text-anchor", "end")
      .text(yLabel);

  var ditem = svg.selectAll(".ditem")
      .data(ditems)
    .enter().append("g")
      .attr("class", "ditem");

  ditem.append("path")
      .attr("class", "line")
      .attr("d", function(d) { return line(d.values); })
      .style("stroke", function(d) { return color(d.name); });

  ditem.append("text")
      .datum(function(d) { return {name: d.name, value: d.values[d.values.length - 1]}; })
      .attr("transform", function(d) { return "translate(" + x(d.value.date) + "," + y(d.value.val) + ")"; })
      .attr("x", 3)
      .attr("dy", ".35em")
      .text(function(d) { return d.name; });

    setInterval(function () {
      d3.json(url, function (error, data) {
        color.domain(d3.keys(data[0]).filter(function(key) { return key !== "date"; }));

        data.forEach(function(d) {
          d.date = parseDate(d.date);
        });

        var ditems = color.domain().map(function(name) {
          return {
            name: name,
            values: data.map(function(d) {
              return {date: d.date, val: +d[name]};
            })
          };
        });

        x.domain(d3.extent(data, function(d) { return d.date; }));

        y.domain([
          d3.min(ditems, function(c) { return d3.min(c.values, function(v) { return v.val; }); }),
          d3.max(ditems, function(c) { return d3.max(c.values, function(v) { return v.val; }); })
        ]);

        var ditem = svg.selectAll(".ditem")
          .data(ditems);

        ditem.select("path")
          .attr("d", function(d) { return line(d.values); });

        ditem.select("text")
          .datum(function(d) { return {name: d.name, value: d.values[d.values.length - 1]}; })
          .attr("transform", function(d) { return "translate(" + x(d.value.date) + "," + y(d.value.val) + ")"; })
          .text(function(d) { return d.name; });

        svg.select(".x.axis")
          .call(xAxis);
        svg.select(".y.axis")
          .call(yAxis);
      });
    }, updateInterval);
  });
}
