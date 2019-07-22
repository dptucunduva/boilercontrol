#!/usr/bin/env nodejs

// Module setup
var express = require('express');
var app = express();
var http = require('http');
var io = require('socket.io-client');
var fs = require('fs');
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline')

// Actuator connections config
//var actTtyPort = '/dev/ttyACM0';
var actTtyPort = 'COM3';
var actTtyBaud = 115200;
const port = new SerialPort(actTtyPort, {baudRate:actTtyBaud});
const parser = port.pipe(new Readline({ delimiter: '\r\n\r\n' }));

// Connect to web agent.
const authProp = JSON.parse(fs.readFileSync('auth.json','utf8'));
webAgent = io.connect('https://italopulga.ddns.net:8099',
	{
		secure:true,
		transportOptions: {
			polling: {
				extraHeaders: {
					'authorization': authProp.authorization
				}
			}
		}
	}
);

// Enrich data from arduino
function addExtraData(data) {
	var jsonData = JSON.parse(data);
	var date = new Date();
    var hours = date.getHours();
    var minutes = date.getMinutes();
    var seconds = date.getSeconds();
    minutes = minutes < 10 ? '0'+minutes : minutes;
    seconds = seconds < 10 ? '0'+seconds : seconds;
	jsonData.updateDt = hours + ':' + minutes + ':' + seconds;
	return JSON.stringify(jsonData);
}

//  Everytime the actuator sends data, we get it and post it to the website
parser.on('data', function(data) {
	try {
		webAgent.emit('tempData', addExtraData(data));
	} catch (e) {
		console.log("Error parsing data: " + data);
	}
});

// Update data each 5s
function getData() {
	port.write("GET /");
}
getData();
setInterval(getData, 5000);

// Command handling
// Heater on
webAgent.on('heaterOn', function(timing) {
	port.write("PUT /heater/on/" + timing);
});

// Heater off
webAgent.on('heaterOff', function (timing) {
	port.write("PUT /heater/off/" + timing);
});

// Heater auto
webAgent.on('heaterAuto', function () {
	port.write("PUT /heater/auto");
});

// Pump off
webAgent.on('pumpOff', function (timing) {
	port.write("PUT /pump/off/" + timing);
});

// Pump on
webAgent.on('pumpOn', function (timing) {
	port.write("PUT /pump/on/" + timing);
});

// Pump auto
webAgent.on('pumpAuto', function () {
	port.write("PUT /pump/auto");
});
