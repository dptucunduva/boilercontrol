const ChartjsNode = require('chartjs-node');
const MongoClient = require('mongodb').MongoClient;
const assert = require('assert');
const fs = require('fs');

const url = 'mongodb://localhost:57017';
const temperatureDbName = 'temperaturedb';
const powerDbName = 'powerdb';
const temperatureClient = new MongoClient(url, { useNewUrlParser: true });
const powerClient = new MongoClient(url, { useNewUrlParser: true });

// Build charts. First temperature charts then power charts
temperatureClient.connect(function(err) {
  assert.equal(null, err);
  const db = temperatureClient.db(temperatureDbName);

    // Temperature, last 3h
    getTemperatureData({timestamp: {$gt : Date.now()-(86400000/8)}}, db).then(chartJsOptions => {return draw(chartJsOptions, 'temperature-3h')})
    // Temperature, last 24h
    .then(() => {return getTemperatureData({timestamp: {$gt : Date.now()-(86400000)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'temperature-24h')} )
    // Temperature, last 3d
    .then(() => {return getTemperatureData({timestamp: {$gt : Date.now()-(86400000*3)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'temperature-3d')} )
    // Temperature, last 7d
    .then(() => {return getTemperatureData({timestamp: {$gt : Date.now()-(86400000*7)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'temperature-7d')} )
    // Temperature, last 30d
    .then(() => {return getTemperatureData({timestamp: {$gt : Date.now()-(86400000*30)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'temperature-30d')} )
    // finish
    .then(() => {return temperatureClient.close()} )
    // Now power charts
    .then(() => {
        // Power charts
        return powerClient.connect(function(err) {
            assert.equal(null, err);
            const db = powerClient.db(powerDbName);

            // Power, last 3h
            getPowerData({timestamp: {$gt : Date.now()-(86400000/8)}}, db).then(chartJsOptions => {return draw(chartJsOptions, 'power-3h')})
            // Power, last 24h
            .then(() => {return getPowerData({timestamp: {$gt : Date.now()-(86400000)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'power-24h')} )
            // Power, last 3d
            .then(() => {return getPowerData({timestamp: {$gt : Date.now()-(86400000*3)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'power-3d')} )
            // Power, last 7d
            .then(() => {return getPowerData({timestamp: {$gt : Date.now()-(86400000*7)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'power-7d')} )
            // Power, last 30d
            .then(() => {return getPowerData({timestamp: {$gt : Date.now()-(86400000*30)}}, db)} ).then(chartJsOptions => {return draw(chartJsOptions, 'power-30d')} )
            // finish
            .then(() => {return powerClient.close()} );
        });
    });
});

// Query temperature database
function getTemperatureData(query, db) {
    return new Promise((resolve, reject) => {
        console.log('Starting query temperature db: ' + JSON.stringify(query));
        const col = db.collection('readings');
        var chartJsOptions = getChartOpts();
        var datasetBoiler = getDataset("Temperatura Boiler","rgb(75, 192, 192)");
        var datasetPanel = getDataset("Temperatura Placas","rgb(255, 192, 192)");
        col.find(query).forEach(function(doc) {
            var dataentryBoiler = doc.boilerTemperature;
            datasetBoiler.data.push(dataentryBoiler);
            var dataentryPanel = doc.solarPanelTemperature;
            datasetPanel.data.push(dataentryPanel);
            chartJsOptions.data.labels.push(doc.date);
        },function(err) {
            // FIXME: We are ignoring all errors
            chartJsOptions.data.datasets.push(datasetBoiler);
            chartJsOptions.data.datasets.push(datasetPanel);
            console.log('Done query temperature db: ' + JSON.stringify(query));
            if (err) {
                reject(err);
            } else {
                resolve(chartJsOptions);
            }
        });
    });
}

// Query power database
function getPowerData(query, db) {
    return new Promise((resolve, reject) => {
        console.log('Starting query power db: ' + JSON.stringify(query));
        const col = db.collection('readings');
        var chartJsOptions = getChartOpts();
        var datasets = [];
        datasets.push(getDataset("1","rgb(75, 192, 192)"));
        datasets.push(getDataset("2","rgb(255, 192, 192)"));
        datasets.push(getDataset("3","rgb(255, 0, 255)"));
        datasets.push(getDataset("4","rgb(64, 64, 64)"));
        datasets.push(getDataset("5","rgb(0, 255, 255)"));
        datasets.push(getDataset("6","rgb(0, 0, 255)"));
        datasets.push(getDataset("7","rgb(75, 0, 75)"));
        datasets.push(getDataset("8","rgb(255, 255, 0)"));
        datasets.push(getDataset("9","rgb(0, 255, 0)"));
        datasets.push(getDataset("10","rgb(255, 0, 0)"));
        datasets.push(getDataset("11","rgb(255, 192, 192)"));
        col.find(query).forEach(function(doc) {
            var dataentry = doc.power;
            datasets[parseInt(doc.circuitId)-1].data.push(dataentry);
            datasets[parseInt(doc.circuitId)-1].label = doc.name;
            if (doc.circuitId == "01") {
                chartJsOptions.data.labels.push(doc.date);
            }
        },function(err) {
            // FIXME: We are ignoring all errors
            chartJsOptions.data.datasets = datasets;
            console.log('Done query power db: ' + JSON.stringify(query));
            if (err) {
                reject(err);
            } else {
                resolve(chartJsOptions);
            }
        });
    });
}

// Build base chart object
function getChartOpts() {
    var chartJsOptions = {
        type: 'line',
        data: {
            labels: [],
            datasets:[]
        },
        options: {
            scales: {
                yAxes: [{
                    ticks: {
                        beginAtZero:false
                    }
                }]
            }
        }
    };
    return chartJsOptions;
}

// Build base dataset object
function getDataset(name, color) {
    var dataset = {};
    dataset.label = name;
    dataset.lineTension = 0.1;
    dataset.borderColor = color;
    dataset.fill = false;
    dataset.pointRadius = 0;
    dataset.borderWidth = 1;
    dataset.cubicInterpolationMode = "monotone";
    dataset.data = [];
    return dataset;
}

// Draw a chart
function draw(drawOptions, filename) {
    var chartNode = new ChartjsNode(1600, 800);
    return chartNode.drawChart(drawOptions)
    .then(() => {
        console.log('Starting drawing ' + filename);
        return chartNode.getImageBuffer('image/png');
    })
    .then(buffer => {
        Array.isArray(buffer)
        return chartNode.getImageStream('image/png');
    })
    .then(streamResult => {
        fs.writeFile('../web/public/charts/' + filename + '.html', '<html><head/><body bgcolor="#FFFFFF"><img src="'+filename+'.png"></body></html>');
        return chartNode.writeImageToFile('image/png', '../web/public/charts/' + filename + '.png');
    })
    .then(() => {
        console.log('Finished drawing ' + filename);
        return chartNode.destroy();
    });
}