#!/usr/bin/env nodejs

// Module setup
var express = require('express');
var app = express();
var http = require('http');
var io = require('socket.io-client');
var fs = require('fs');
var SunCalc = require('suncalc');

// Broker connection config
var mqtt = require('mqtt');
var mqttClient  = mqtt.connect('mqtt://mqtt.home', {clientId: 'boiler-control-unit'});

// Control variables
var cycle = "auto";
var currentStatus = {};
var nextPublish;

// Connect to MQTT server
mqttClient.on('connect', function () {
	mqttClient.subscribe('/boiler/sensor/#');
});

//  Everytime the actuator sends data, we get it and post it to the website
mqttClient.on('message', function (topic, message) {
	switch (topic) {
		case '/boiler/sensor/reservoir/temperature':
			currentStatus.reservoirTemp = message.toString();
			break;
		case '/boiler/sensor/solarpanel/temperature':
			currentStatus.solarPanelTemp = message.toString();
			break;
		case '/boiler/sensor/heater/status':
			currentStatus.heaterStatus = message.toString();
			break;
		case '/boiler/sensor/heater/ontemp':
			currentStatus.heaterOnTemp = message.toString();
			break;
		case '/boiler/sensor/heater/offtemp':
			currentStatus.heaterOffTemp = message.toString();
			break;
		case '/boiler/sensor/heater/override':
			currentStatus.heaterOverride = message.toString() == "disabled" ? "disabled" : new Date(Date.now() + parseInt(message.toString())*1000)
			break;
		case '/boiler/sensor/pump/status':
			currentStatus.pumpStatus = message.toString();
			break;
		case '/boiler/sensor/pump/override':
			currentStatus.pumpOverride = message.toString() == "disabled" ? "disabled" : new Date(Date.now() + parseInt(message.toString())*1000)
			break;
		case '/boiler/sensor/cycle/status':
			currentStatus.cycleStatus = message.toString();
			break;
		default:
			console.log("Topic unknown");
	}

	// Force immediate publish
	nextPublish = Date.now();
});

// Enrich data before publishing it
function addExtraData(data) {
	var date = new Date();
    var hours = date.getHours();
    var minutes = date.getMinutes();
    var seconds = date.getSeconds();
    minutes = minutes < 10 ? '0'+minutes : minutes;
    seconds = seconds < 10 ? '0'+seconds : seconds;
	data.updateDt = hours + ':' + minutes + ':' + seconds;

	// cycle
	if (cycle == "auto") {
		data.cycleAuto = true;
	} else {
		data.cycleAuto = false;
	}

	return data;
}

// Publish data each second if it has changed
function getData() {
	if (Date.now() >= nextPublish) {
		dataToSend = addExtraData(currentStatus);
		webAgent.emit('tempData',dataToSend);

		// Publish at least once every 30s
		nextPublish = Date.now() + 30000;
	}
}
getData();
setInterval(getData, 1000);

function checkSunTime() {
	// São José dos Campos coordinates
	var sunData = SunCalc.getTimes(new Date(), -23.1823096, -45.9502316);
	var sunrise = sunData.sunriseEnd;
	var sunset = sunData.sunsetStart;
	var now = new Date();

	sunrise.setHours(sunrise.getHours() + 2);
	sunset.setHours(sunset.getHours() - 1.5);

	if (cycle == "on") {
		mqttClient.publish('/boiler/actuator/cycle/setstatus','enable');
	} else if (cycle == "off") {
		mqttClient.publish('/boiler/actuator/cycle/setstatus','disable');
	} else if (cycle == "auto") {
		if (now > sunrise  && now < sunset) {
			mqttClient.publish('/boiler/actuator/cycle/setstatus','enable');
		} else {
			mqttClient.publish('/boiler/actuator/cycle/setstatus','disable');
		}
	}
}
checkSunTime();
setInterval(checkSunTime, 17000);


// Connect to web agent.
const authProp = JSON.parse(fs.readFileSync('auth.json','utf8'));
webAgent = io.connect('https://italopulga.ddns.net:8099',
	{
		secure:true,
		reconnection: true,
    	reconnectionDelay: 1000,
    	reconnectionDelayMax : 5000,
    	reconnectionAttempts: Infinity,
		transportOptions: {
			polling: {
				extraHeaders: {
					'authorization': authProp.authorization
				}
			}
		}
	}
);

// Command handling
// Heater on
webAgent.on('heaterOn', function(timing) {
	mqttClient.publish('/boiler/actuator/heater/setoverride','enable,'+timing);
});

// Heater off
webAgent.on('heaterOff', function (timing) {
	mqttClient.publish('/boiler/actuator/heater/setoverride','disable,'+timing);
});

// Heater auto
webAgent.on('heaterAuto', function () {
	mqttClient.publish('/boiler/actuator/heater/setoverride','auto');
});

// Pump regular cycling
// Cycling on
webAgent.on('cycleOn', function(timing) {
	cycle = "on";
	checkSunTime();
});

// Cycling off
webAgent.on('cycleOff', function (timing) {
	cycle = "off";
	checkSunTime();
});

// Cycling auto
webAgent.on('cycleAuto', function () {
	cycle = "auto";
	checkSunTime();
});

// Pump off
webAgent.on('pumpOff', function (timing) {
	mqttClient.publish('/boiler/actuator/pump/setoverride','disable,'+timing);
});

// Pump on
webAgent.on('pumpOn', function (timing) {
	mqttClient.publish('/boiler/actuator/pump/setoverride','enable,'+timing);
});

// Pump auto
webAgent.on('pumpAuto', function () {
	mqttClient.publish('/boiler/actuator/pump/setoverride','auto');
});
