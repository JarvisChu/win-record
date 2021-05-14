var events = require('events')
var bindings = require('bindings')
var Record = bindings('win_record_addon').Record

module.exports = function () {
  var that = new events.EventEmitter()
  var record = null
  var left = false
  var right = false

  that.ref = function () {
    if (record) record.ref()
  }

  that.unref = function () {
    if (record) record.unref()
  }

  that.destroy = function () {
    if (record) record.destroy()
    record = null
  }

  that.start = function(...props) {
    console.info("start")
    if(record == null){
      record = new Record(function (type, x) {
        if(type == 'start') that.emit(type);
        else if(type == 'stop') that.emit(type);
        else that.emit(type, x);
      }, ...props)
    }
  }

  return that
}
