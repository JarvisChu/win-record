var record = require('../')

var r = record()
var r1 = record()

r.on('start', function () {
  console.log('start')
})

r.on('stop', function () {
  console.log('stop')
})

r.on('localaudio', function (data) {
  console.log('localaudio: ', data)
})

r.on('remoteaudio', function (data) {
  console.log('remoteaudio: ', data)
})

r.on('error', function (msg) {
  console.log('error', msg)
})

r.start(1, 8000, 16, 1);

setTimeout(function () {
  r.destroy()
  console.log('destroy')
}, 3000)
