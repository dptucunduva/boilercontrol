#!/usr/bin/env nodejs

// Module setup
const express = require('express');
const app = express();
const http = require('http');
const io = require('socket.io-client');
const fs = require('fs');
const SunCalc = require('suncalc');

// Telegram Bot
var telegramEnabled = true;
var nextTelegramNotification = Date.now();
var bot;
var telegramJson;
if (fs.existsSync('telegram.json')) {
	telegramJson = JSON.parse(fs.readFileSync('telegram.json','utf8'));
	const TeleBot = require('telebot');
	bot = new TeleBot(telegramJson.apiKey);
	bot.on('*', (msg) => {
		return bot.sendMessage(msg.from.id, `Hello, ${ msg.from.first_name }! This is a private bot, no commands are available. No unauthorized access is allowed, sorry. Thank you!`);
	});
	bot.start();
	telegramEnabled = true;
}

// Broker connection config
const mqtt = require('mqtt');
const mqttClient  = mqtt.connect('mqtt://mqtt.home', {clientId: 'boiler-control-unit'});

// Control variables
var cycle = "auto";
var currentStatus = {};
var nextPublish;
var lastTempChange = Date.now();

// Connect to MQTT server
mqttClient.on('connect', function () {
	mqttClient.subscribe('/boiler/sensor/#');
});

//  Everytime the actuator sends data, we get it and post it to the website
mqttClient.on('message', function (topic, message) {
	switch (topic) {
		case '/boiler/sensor/reservoir/temperature':
			var newReservoirTemp = message.toString();
			if (newReservoirTemp != currentStatus.reservoirTemp) {
				lastTempChange = Date.now();
			}
			currentStatus.reservoirTemp = newReservoirTemp;
			break;
		case '/boiler/sensor/solarpanel/temperature':
			var newSolarPanelTemp = message.toString();
			if (newSolarPanelTemp != currentStatus.solarPanelTemp) {
				lastTempChange = Date.now();
			}
			currentStatus.solarPanelTemp = newSolarPanelTemp;
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
	data.updateDt = formatTime(new Date());

	// cycle
	if (cycle == "auto") {
		data.cycleAuto = true;
	} else {
		data.cycleAuto = false;
	}

	//Check if the temperature has changed during the last 10 minutes
	if (lastTempChange < Date.now() - 1000*60*10) {
		data.problem = true;
	}else {
		data.problem = false;
	}
	data.lastTempChange = formatTime(new Date(lastTempChange));

	// Send message to telegram bot
	if (telegramEnabled && data.problem && nextTelegramNotification < Date.now()) {
		bot.sendMessage(telegramJson.id, "System problem! Last temp update was at " + data.lastTempChange);
		nextTelegramNotification = Date.now() + 1000*60*10; // Warn once every 10 minutes.
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

function formatTime(date) {
    var hours = date.getHours();
    var minutes = date.getMinutes();
    var seconds = date.getSeconds();
    minutes = minutes < 10 ? '0'+minutes : minutes;
    seconds = seconds < 10 ? '0'+seconds : seconds;
	return hours + ':' + minutes + ':' + seconds;
}

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