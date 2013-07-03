var path = require('path'),
    http = require('http');

exports.binary = path.join(__dirname, '..', 'aeternum');
exports.pidFile = path.join(__dirname, 'tmp', 'test-child-' + process.pid + '.pid');

exports.request = function (port, cb) {
  var req = http.request({
    host: 'localhost',
    port: port
  });

  req.on('error', cb);
  req.on('response', function () {
    cb();
  });

  req.end();
};

exports.restart = function (port, cb) {
  var req = http.request({
    host: 'localhost',
    port: port,
    path: '/exit'
  });

  req.on('error', cb);
  req.on('response', function () {
    cb();
  });

  req.end();
};
