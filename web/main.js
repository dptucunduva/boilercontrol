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
httpServer.listen(8080);

// HTTPS server
var sslProp = JSON.parse(fs.readFileSync('cert.json','utf8'));
const credentials = {
	key: fs.readFileSync(sslProp.key, 'utf8'),
	cert: fs.readFileSync(sslProp.cert, 'utf8'),
	ca: fs.readFileSync(sslProp.ca, 'utf8')
};
const httpsServer = https.createServer(credentials, app)
httpsServer.listen(8443);

// Authentication
const authProp = JSON.parse(fs.readFileSync('auth.json','utf8'));
app.use('/',basicAuth(authProp));
function checkSocketIoAuth(socket, token) {
    var auth = authProp.authorization;
    if (typeof token == 'undefined' ||
        typeof auth == 'undefined' ||
        decodeURIComponent(token).indexOf(auth) < 0) {
            socket.disconnect();
            console.log('Invalid connection detected. Closing it.');
            return false;
    }
    return true;
}

// Browser socket. Send information to the browser
const ioBrowserServer = https.createServer(credentials);
ioBrowserServer.listen(8098);
var browser = require('socket.io')(ioBrowserServer);
browser.on('connect', function(socket) {
    checkSocketIoAuth(socket, socket.handshake.headers.cookie);
})

// Admin socket. Wait for connections from internal agent with current data
var actuatorData;
const ioAdminServer = https.createServer(credentials);
ioAdminServer.listen(8099);
var admin = require('socket.io')(ioAdminServer);
admin.on('connect', function(socket) {
    if (checkSocketIoAuth(socket, socket.handshake.headers.authorization)) {
        socket.on('tempData', function(data) {
            actuatorData = data;
            browser.emit('tempData', actuatorData);
        })
    }
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

// Cycle on
app.put('/cycle/on', function (req, res) {
	admin.emit('cycleOn');
	writeData(req, res);
})

// Cycle off
app.put('/cycle/off', function (req, res) {
	admin.emit('cycleOff');
	writeData(req, res);
})

// Cycle auto
app.put('/cycle/auto', function (req, res) {
	admin.emit('cycleAuto');
	writeData(req, res);
})

// Pump on
app.put('/pump/on/:time', function (req, res) {
	admin.emit('pumpOn', req.params.time);
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
