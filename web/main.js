#!/usr/bin/env nodejs

// Module setup
var express = require('express');
var basicAuth = require('express-basic-auth')
var http = require('http');
var io = require('socket.io')(http.Server(app));
var app = express();

// Websocket control and data.
var actuatorData;

// Browser socket. Send information to the browser
var browser = io.listen(8098);

// Admin socket. Wait for connections from internal agent with current data
var admin = io.listen(8099);
admin.on('connect', function(socket) {
	socket.on('tempData', function(data) {
		actuatorData = data;
		browser.emit('tempData', actuatorData);
	})
})

// Authentication
app.use('/',
	basicAuth(
		{
			users: { 'xxx': 'yyy' },
			challenge: true,
			realm: 'XXXXXXXXXX'
		}
	)
);

// Public static files
app.use(express.static('public'));

// Write current data
function writeData(res) {
	res.setHeader('Content-Type', 'application/json');
	res.setHeader('Cache-Control', 'no-cache');
	res.setHeader('Connection', 'closed');
	res.send(actuatorData);
}

// Get current status
app.get('/status', function (req, res) {
	writeData(res);
})

// Heater on
app.put('/heater/on/:time', function (req, res) {
	admin.emit('heaterOn', req.params.time);
	writeData(res);
})

// Heater off
app.put('/heater/off/:time', function (req, res) {
	admin.emit('heaterOff', req.params.time);
	writeData(res);
})

// Heater auto
app.put('/heater/auto', function (req, res) {
	admin.emit('heaterAuto');
	writeData(res);
})

// Pump off
app.put('/pump/off/:time', function (req, res) {
	admin.emit('pumpOff', req.params.time);
	writeData(res);
})

// Pump auto
app.put('/pump/auto', function (req, res) {
	admin.emit('pumpAuto');
	writeData(res);
})

// HTTP server
var server = app.listen(80);