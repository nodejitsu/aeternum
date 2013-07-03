var http = require('http'),
    port = parseInt(process.argv[2], 10) || 8123;

http.createServer(function (req, res) {
  console.log('request');

  res.writeHead(200, { 'content-type': 'text/plain' });
  res.end('Hello, aeternum!\n');

  if (req.url === '/exit') {
    process.exit();
  }
}).listen(port);
