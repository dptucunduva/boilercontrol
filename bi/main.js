#!/usr/bin/env nodejs

const fs = require('fs');
const assert = require('assert');
const MongoClient = require('mongodb').MongoClient;

// Data files location
const powerDatadir = '../internal/data/power';
const temperatureDatadir = '../internal/data/temperature';

// Mongo
const mdbUrl = 'mongodb://italopulga.ddns.net:57017';
const mdbClient = new MongoClient(mdbUrl, { useNewUrlParser: true });

// Handle files
function loadData() {

	mdbClient.connect(function(err) {
		assert.equal(null, err);
		console.log('Starting...');

		// For each file in power folder
		fs.readdirSync(powerDatadir).forEach(file => {
	  		var powerData = JSON.parse(fs.readFileSync(powerDatadir+'/'+file,'utf8'));
	  		addToDB('powerdb', 'readings', powerData, powerDatadir+'/'+file);
		});

		// For each file in temperature folder
		fs.readdirSync(temperatureDatadir).forEach(file => {
	  		var temperatureData = JSON.parse(fs.readFileSync(temperatureDatadir+'/'+file,'utf8'));
	  		addToDB('temperaturedb', 'readings', temperatureData, temperatureDatadir+'/'+file);
		});

		console.log('Loaded.');
		mdbClient.close();
	});
}
loadData();
setInterval(loadData, 60000);

// Add entry to DB
function addToDB(dbName, collectionName, data, fileToDel) {
	const collection = mdbClient.db(dbName).collection(collectionName);
	collection.insertOne(data, function(err, result) {
		assert.equal(err, null);
		assert.equal(1, result.result.n);
		assert.equal(1, result.ops.length);
		console.log('Inserted: ' + JSON.stringfy(data));
	  	fs.unlinkSync(fileToDel);
	});
}