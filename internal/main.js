// Module setup
var express = require('express');
var app = express();
var http = require('http');
var io = require('socket.io-client');

// Actuator connections config
var actHost = '192.168.1.211';
var actPort = 80;
var actuatorGetDataOptions = {
	host: actHost,
	port: actPort,
	path: '/',
	method: 'GET'
}
var actuatorEnableHeater = {
	host: actHost,
	port: actPort,
	method: 'PUT'
}
var actuatorDisableHeater = {
	host: actHost,
	port: actPort,
	method: 'PUT'
}
var actuatorAutoHeater = {
	host: actHost,
	port: actPort,
	path: '/heater/auto',
	method: 'PUT'
}
var actuatorDisablePump = {
	host: actHost,
	port: actPort,
	method: 'PUT'
}
var actuatorAutoPump = {
	host: actHost,
	port: actPort,
	path: '/pump/auto/',
	method: 'PUT'
}

// Connect to web agent.
// FIXME: Nomes para conexão ao invés de IPs
webAgent = io.connect('http://127.0.0.1:8099');

// Retrieve data from actuator each 3s
var currentData;
function getData() {
	currentData = '';
	http.request(actuatorGetDataOptions, function(res) {
		res.on('data', function (chunk) {
    		currentData += chunk;
			webAgent.emit('tempData', currentData);
		});
	}).end();
}
setInterval(getData, 1000);

// Command handling
// Heater on
webAgent.on('heaterOn', function(timing) {
	actuatorEnableHeater.path = '/heater/on/'+timing;
	http.request(actuatorEnableHeater, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
			webAgent.emit('tempData', currentData);
		})
	}).end();
})

// Heater off
webAgent.on('heaterOff', function (timing) {
	actuatorDisableHeater.path = '/heater/off/'+timing;
	http.request(actuatorDisableHeater, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
			webAgent.emit('tempData', currentData);
		})
	}).end();
})

// Heater auto
webAgent.on('heaterAuto', function (dummy) {
	http.request(actuatorAutoHeater, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
			webAgent.emit('tempData', currentData);
		})
	}).end();
})

// Pump off
webAgent.on('pumpOff', function (timing) {
	actuatorDisablePump.path = '/pump/off/'+timing;
	http.request(actuatorDisablePump, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
			webAgent.emit('tempData', currentData);
		})
	}).end();
})

// Pump auto
webAgent.on('pumpAuto', function (dummy) {
	http.request(actuatorAutoPump, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
			webAgent.emit('tempData', currentData);
		})
	}).end();
})
