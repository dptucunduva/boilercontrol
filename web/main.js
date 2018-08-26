#!/usr/bin/env nodejs

// Module setup
var express = require('express');
var basicAuth = require('express-basic-auth')
var https = require('https');
var http = require('http');
var fs = require('fs');
var cookieParser = require('cookie-parser');
var app = express();
app.use(cookieParser());
var appRedirect = express();

// HTTP Server. Just redirect to HTTPS
const httpServer = http.createServer(appRedirect)
appRedirect.get('*', function(req, res) {  
    res.redirect('https://' + req.headers.host + req.url);
})
httpServer.listen(80);

// HTTPS server
var sslProp = JSON.parse(fs.readFileSync('cert.json','utf8'));
const credentials = {
	key: fs.readFileSync(sslProp.key, 'utf8'),
	cert: fs.readFileSync(sslProp.cert, 'utf8'),
	ca: fs.readFileSync(sslProp.ca, 'utf8')
};
const httpsServer = https.createServer(credentials, app)
httpsServer.listen(443);

// Authentication
const authProp = JSON.parse(fs.readFileSync('auth.json','utf8'));
app.use('/',basicAuth(authProp));

// Browser socket. Send information to the browser
const ioBrowserServer = https.createServer(credentials)
ioBrowserServer.listen(8098);
var browser = require('socket.io')(ioBrowserServer);
browser.on('connect', function(socket) {
	var ioCookieHeader = socket.handshake.headers.cookie;
	var auth = authProp.authorization;
	if (typeof ioCookieHeader == 'undefined' ||
	    typeof auth == 'undefined' ||
	    decodeURIComponent(ioCookieHeader).indexOf(auth) <= 0) {
		socket.destroy();
	}
})

// Admin socket. Wait for connections from internal agent with current data
var actuatorData;
const ioAdminServer = https.createServer(credentials)
ioAdminServer.listen(8099);
var admin = require('socket.io')(ioAdminServer);
admin.on('connect', function(socket) {
	socket.on('tempData', function(data) {
		actuatorData = data;
		browser.emit('tempData', actuatorData);
	})
})

// Public static files
app.use(express.static('public'));

// Write current data
function writeData(req, res) {
	res.setHeader('Content-Type', 'application/json');
	res.setHeader('Cache-Control', 'no-cache');
	res.setHeader('Connection', 'closed');
	res.cookie('auth', req.headers.authorization, {httpOnly: true});
	res.send(actuatorData);
}

// Get current status
app.get('/status', function (req, res) {
	writeData(req, res);
})

// Heater on
app.put('/heater/on/:time', function (req, res) {
	admin.emit('heaterOn', req.params.time);
	writeData(req, res);
})

// Heater off
app.put('/heater/off/:time', function (req, res) {
	admin.emit('heaterOff', req.params.time);
	writeData(req, res);
})

// Heater auto
app.put('/heater/auto', function (req, res) {
	admin.emit('heaterAuto');
	writeData(req, res);
})

// Pump off
app.put('/pump/off/:time', function (req, res) {
	admin.emit('pumpOff', req.params.time);
	writeData(req, res);
})

// Pump auto
app.put('/pump/auto', function (req, res) {
	admin.emit('pumpAuto');
	writeData(req, res);
})
