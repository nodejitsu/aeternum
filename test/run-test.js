var assert = require('assert'),
    spawn = require('child_process').spawn,
    helpers = require('./helpers'),
    cb = require('assert-called'),
    PORT = 8400,
    data = '',
    child,
    pid;

child = spawn(
  helpers.binary,
  ['run', '--', 'node', 'test/fixtures/server.js', PORT.toString()]
);

setTimeout(function () {
  // Wait some time for the process to be brought up.
  helpers.request(PORT, cb(function (err) {
    console.log('first req', err);
    assert(!err);

    helpers.restart(PORT, cb(function (err) {
      console.log('second req', err);
      assert(!err);

      setTimeout(function () {
        // Wait some time for the process to be brought up.
        helpers.request(PORT, cb(function (err) {
          console.log('third req', err);
          assert(!err);

          child.kill();
          process.exit();
        }));
      }, 1000);
    }));
  }));
}, 1000);
