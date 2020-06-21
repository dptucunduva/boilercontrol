// MongoDB
const assert = require('assert');
const MongoClient = require('mongodb').MongoClient;
const mdbUrl = 'mongodb://localhost:27017';
const mdbClient = new MongoClient(mdbUrl, { useNewUrlParser: true, useUnifiedTopology: true });

// constants
const dbName = 'temperature';
const collectionName = 'readings';

module.exports = {
    // Save it to mongo database
    saveIt: function(data) {
        const collection = mdbClient.db(dbName).collection(collectionName);
        collection.insertOne(data, function(err, result) {
            assert.equal(err, null);
            assert.equal(1, result.result.n);
            assert.equal(1, result.ops.length);
        });
    }
}

mdbClient.connect(function(err) {
    assert.equal(null, err);
});