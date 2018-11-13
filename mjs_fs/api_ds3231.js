let DS3231DateTime = {
  _cr: ffi('void* mgos_ds3231_date_time_create()'),
  _fr: ffi('void mgos_ds3231_date_time_free(void*)'),
  _set_date: ffi('void mgos_ds3231_date_time_set_date(void*, int, int, int)'),
  _set_time: ffi('void mgos_ds3231_date_time_set_time(void*, int, int, int)'),
  _set_unix: ffi('void mgos_ds3231_date_time_set_unixtime(void*, int)'),
  _get_unix: ffi('int mgos_ds3231_date_time_get_unixtime(void*)'),
  _descr: ffi('void* mgos_ds3231_date_time_get_struct_descr()'),
  // ## **`DS3231DateTime.create()`**
  // Creates a DS3231DateTime instance to be used for reading/wrtting data
  // from/to DS3231.
  // Return value: an object with the methods described below.
  create: function() {
    let obj = Object.create(DS3231DateTime._proto);
    obj.data = DS3231DateTime._cr();
    obj.owned = true;
    return obj;
  },
  createFromPointer: function(ptr) {
    let obj = Object.create(DS3231DateTime._proto);
    obj.data = ptr;
    obj.owned = false;
    return obj;
  },
  _proto: {
    // ## **`dsData.free()`**
    // Frees a DS3231DateTime instance.
    // No methods can be called on this instance after that.
    // Return value: none.
    free: function() {
      if (this.owned) {
        DS3231DateTime._fr(this.data);
      }
    },
    setDate: function(y, m, d) {
      DS3231DateTime._set_date(this.data, y, m, d);
    },
    setTime: function(h, m, s) {
      DS3231DateTime._set_time(this.data, h, m, s);
    },
    getUnixtime: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.unixtime;
    },
    getYear: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Year + 2000;
    },
    getMonth: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Month;
    },
    getDay: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Day;
    },
    getDow: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Dow;
    },
    getHour: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Hour;
    },
    getMinute: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Minute;
    },
    getSecond: function() {
      let o = s2o(this.data, DS3231DateTime._descr());
      return o.Second;
    },
    setUnixtime: function(u) {
      DS3231DateTime._set_unix(this.data, u);
    }
  }
};

let DS3231 = {
  // Alarm 1
  ALARM_EVERY_SECOND: 0xf1,                   // (0B1111 0001),
  ALARM_MATCH_SECOND: 0x71,                   // (0B0111 0001),
  ALARM_MATCH_SECOND_MINUTE: 0x31,            // (0B0011 0001),
  ALARM_MATCH_SECOND_MINUTE_HOUR: 0x11,       // (0B0001 0001),
  ALARM_MATCH_SECOND_MINUTE_HOUR_DATE: 0x01,  // (0B0000 0001),
  ALARM_MATCH_SECOND_MINUTE_HOUR_DOW: 0x09,   // (0B0000 1001),

  ALARM_HOURLY: 0x33,   // (0B0011 0011),
  ALARM_DAILY: 0x13,    // (0B0001 0011),
  ALARM_WEEKLY: 0x0b,   // (0B0000 1011),
  ALARM_MONTHLY: 0x03,  // (0B0000 0011),

  // Alarm 2
  ALARM_EVERY_MINUTE: 0x72,            // (0B0111 0010),
  ALARM_MATCH_MINUTE: 0x32,            // (0B0011 0010),
  ALARM_MATCH_MINUTE_HOUR: 0x12,       // (0B0001 0010),
  ALARM_MATCH_MINUTE_HOUR_DATE: 0x02,  // (0B0000 0010),
  ALARM_MATCH_MINUTE_HOUR_DOW: 0x0a,   // (0B0000 1010),

  _cr: ffi('void* mgos_ds3231_create(int)'),
  _del: ffi('void mgos_ds3231_free(void*)'),
  _rd: ffi('void* mgos_ds3231_read(void*)'),
  _wr: ffi('int mgos_ds3231_write(void*, void*)'),
  _wru: ffi('int mgos_ds3231_write_unixtime(void*, int)'),
  _gtfc: ffi('float mgos_ds3231_get_temperature_c(void*)'),
  _gtff: ffi('float mgos_ds3231_get_temperature_f(void*)'),
  _disa: ffi('int mgos_ds3231_disable_alarms(void*)'),
  _cka: ffi('int mgos_ds3231_check_alarms(void*, bool)'),
  _sa: ffi('int mgos_ds3231_set_alarm(void*, void*, int)'),
  _f: ffi('int mgos_strftime(char *, int, char *, int)'),
  _set_tod: ffi('int mgos_ds3231_settimeofday(void*)'),
  _ck_stop: ffi('int mgos_ds3231_check_stopflag(void*, int)'),

  // ## **`DS3231.createI2C(address)`**
  // Creates a DS3231 instance on the I2C bus with the given address `address`.
  // Return value: an object with the methods described below.
  create: function(address) {
    let obj = Object.create(DS3231._proto);
    obj.rtc = DS3231._cr(address);
    return obj;
  },
  _proto: {
    // ## **`rtc.free()`**
    // Frees the  DS3231 instance.
    // No methods can be called on this instance after that.
    // Return value: none.
    free: function() {
      DS3231._del(this.rtc);
    },

    // ## **`rtc.read()`**
    // Reads date/time from the RTC
    // Returns a DS3231DateTime struct.
    read: function() {
      let ptr = DS3231._rd(this.rtc);
      return DS3231DateTime.createFromPointer(ptr);
    },

    // ## **`rtc.write(dt)`**
    // Writes a DS3231DateTime structure
    // Returns `true` on success
    write: function(dt) {
      return DS3231._wr(this.rtc, dt.data);
    },

    // ## **`rtc.writeUnixtime(unixtime)`**
    // Sets the date/time from a `unixtime`
    // Returns `true` on success
    writeUnixtime: function(unixtime) {
      return DS3231._wru(this.rtc, unixtime);
    },

    // ## **`rtc.getTemperatureC()`**
    // Return the temperature in Celsius
    getTemperatureC: function() {
      return DS3231._gtfc(this.rtc);
    },

    // ## **`rtc.getTemperatureF()`**
    // Return the temperature in Fahrenheit
    getTemperatureF: function() {
      return DS3231._gtff(this.rtc);
    },

    // ## **`rtc.disableAlarms()`**
    // Disable alarms
    // Returns `true` on success
    disableAlarms: function() {
      return DS3231._disa(this.rtc);
    },

    // ## **`rtc.checkAlarms()`**
    // Disable alarms
    // Returns:
    //  0 if no alarm
    //  1 if Alarm1 was triggered
    //  2 if Alarm2 was triggered
    //  3 if both alarms were triggered
    checkAlarms: function() {
      return DS3231._cka(this.rtc, false);
    },

    // MFP mod
    // ## **`rtc.checkStopFlag()`**
    // Check the OSF bit
    // Returns:
    //  the oscillator stop flag bit
    checkStopFlag: function(clear) {
      return DS3231._ck_stop(this.rtc, clear);
    },

    // ## **`rtc.setAlarm(alarm)`**
    // Set alarm
    // `dt` - a DS3231DateTime structure
    // `mode` - an alarm mode selected from the ALARM_ constants
    // Returns `true` on success
    setAlarm: function(dt, mode) {
      return DS3231._sa(this.rtc, dt.data, mode);
    },

    // ## **`rtc.setTimeOfDay()`**
    // Set system time
    // Returns 0 on success
    setTimeOfDay: function() {
      return DS3231._set_tod(this.rtc);
    }
  }
};
