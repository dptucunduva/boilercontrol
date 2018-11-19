#!/usr/bin/env nodejs

// Module setup
var express = require('express');
var app = express();
var http = require('http');
var io = require('socket.io-client');
var fs = require('fs');

// Actuator connections config
var actHost = 'arduino';
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

// Circuit name mapping
const nameMap = {
	"01":"Luz muros e quintal",
	"02":"Luz salas, quartos e escritório",
	"03":"Luz cozinha, AS e churrasqueira",
	"04":"Tomadas sala de estar e escritório",
	"05":"Tomadas sala de jantar e TV",
	"06":"Tomadas suíte",
	"07":"Tomadas banheiro suíte, closet suíte master e corredor",
	"08":"Tomadas banheiro suíte master",
	"09":"Tomadas suíte master",
	"10":"Tomadas gerais cozinha",
	"11":"Tomada Microondas",
	"12":"Forno",
	"13":"Lava roupas",
	"14":"Secadora e ferro de passar",
	"15":"Tomadas gerais área de serviço",
	"16":"Tomadas gerais churrasqueira",
	"17":"Controle do boiler",
	"18":"Ar condicionado suíte",
	"19":"Ar condicionado sala de TV",
	"20":"Ar condicionado escritório",
	"21":"Hidromassagem suíte master",
	"22":"Ar condicionado suíte master"
};

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

	// Add names in each circuit
	var ps = {};
	for (const [key, value] of Object.entries(jsonData.powersensors)) {
		ps[key + " - " + nameMap[key]] = value;
	}
	jsonData.powersensors = ps;

	return JSON.stringify(jsonData);
}

// Retrieve data from actuator each 3s
function getData() {
	var currentData = '';
	http.request(actuatorGetDataOptions, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
		});
		res.on('end', function() {
			currentData = addExtraData(currentData);
			webAgent.emit('tempData', currentData);

			// store data in a file so that an agent can send it to a DB
			var readingDate = new Date();

			// Power sensors
			for (const [key, value] of Object.entries(JSON.parse(currentData).powersensors)) {
				var entry = {};
				entry.name = key.substring(5);
				entry.circuitId = key.substring(0,2);
				entry.date = readingDate;
				entry.localizedDate = readingDate.toLocaleString('pt-BR')
				entry.timestamp = readingDate.getTime();
				entry.current = value.current;
				entry.power = value.power;
				fs.writeFileSync('data/power/'+entry.circuitId+'-'+entry.timestamp+'-'+
					entry.name.replace(/[^\w\s]/gi, '').replace(/\s+/g,'-').toLowerCase()+'.json', 
					JSON.stringify(entry), 'utf8');
			}

			// Temperature data sensors
			var temperatureData = JSON.parse(currentData);
			delete temperatureData["powersensors"];
			temperatureData.date = readingDate;
			temperatureData.localizedDate = readingDate.toLocaleString('pt-BR')
			temperatureData.timestamp = readingDate.getTime();
			fs.writeFileSync('data/temperature/'+readingDate.getTime()+'.json', 
				JSON.stringify(temperatureData), 'utf8');
		});
	}).end();
}
getData();
setInterval(getData, 10000);

// Command handling
// Heater on
webAgent.on('heaterOn', function(timing) {
	var currentData = '';
	actuatorEnableHeater.path = '/heater/on/'+timing;
	http.request(actuatorEnableHeater, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
		});
		res.on('end', function() {
			webAgent.emit('tempData', addExtraData(currentData));
		})
	}).end();
})

// Heater off
webAgent.on('heaterOff', function (timing) {
	var currentData = '';
	actuatorDisableHeater.path = '/heater/off/'+timing;
	http.request(actuatorDisableHeater, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
		});
		res.on('end', function() {
			webAgent.emit('tempData', addExtraData(currentData));
		})
	}).end();
})

// Heater auto
webAgent.on('heaterAuto', function (dummy) {
	var currentData = '';
	http.request(actuatorAutoHeater, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
		});
		res.on('end', function() {
			webAgent.emit('tempData', addExtraData(currentData));
		})
	}).end();
})

// Pump off
webAgent.on('pumpOff', function (timing) {
	var currentData = '';
	actuatorDisablePump.path = '/pump/off/'+timing;
	http.request(actuatorDisablePump, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
		});
		res.on('end', function() {
			webAgent.emit('tempData', addExtraData(currentData));
		})
	}).end();
})

// Pump auto
webAgent.on('pumpAuto', function (dummy) {
	var currentData = '';
	http.request(actuatorAutoPump, function(res) {
		res.on('data', function (chunk) {
			currentData += chunk;
		});
		res.on('end', function() {
			webAgent.emit('tempData', addExtraData(currentData));
		})
	}).end();
})
