var events = require('events')
var bindings = require('bindings')
var Record = bindings('win_record_addon').Record

module.exports = function () {
  var that = new events.EventEmitter()
  var record = null

  that.destroy = function () {
    if (record) record.destroy()
    record = null
  }

  that.start = function(...props) {
    console.info("start")
    if(record == null){
      record = new Record(function (type, x) {
        that.emit(type, x);
      }, ...props)
    }
  }

  return that
}
