var Record = require('bindings')('win_record_addon').Record

const r = new Record();
console.log(r.add(1,2))

module.exports = Record