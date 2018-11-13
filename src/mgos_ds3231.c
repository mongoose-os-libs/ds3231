#include <mgos.h>
#include <mjs.h>
#include "mgos_ds3231.h"
#include "mgos_i2c.h"

struct mgos_ds3231_date_time *mgos_ds3231_date_time_create() {
  struct mgos_ds3231_date_time *dt = (struct mgos_ds3231_date_time *) calloc(
      1, sizeof(struct mgos_ds3231_date_time));
  if (NULL == dt) {
    LOG(LL_INFO, ("Could not create \"struct mgos_ds3231_date_time\""));
    return NULL;
  }
  return dt;
}

void mgos_ds3231_date_time_free(struct mgos_ds3231_date_time *dt) {
  if (NULL != dt) {
    free(dt);
  }
}

/*
 * From
 * http://stason.org/TULARC/society/calendars/2-5-What-day-of-the-week-was-2-August-1953.html
 */
static uint8_t dow(uint16_t year, uint8_t month, uint8_t day) {
  uint16_t a = (14 - month) / 12;
  uint16_t y = year - a;
  uint16_t m = month + 12 * a - 2;
  uint8_t d = (day + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12) % 7;

  return (0 == d) ? 7 : d; /* adjust for Sunday=7 */
}

void mgos_ds3231_date_time_set_date(struct mgos_ds3231_date_time *dt,
                                    uint16_t year, uint8_t month, uint8_t day) {
  dt->Year = year - 2000;
  dt->Month = month;
  dt->Day = day;
  dt->Dow = dow(year, month, day);
}

const struct mjs_c_struct_member *mgos_ds3231_date_time_get_struct_descr() {
  static const struct mjs_c_struct_member struct_descr[] = {
      {"Second", offsetof(struct mgos_ds3231_date_time, Second),
       MJS_FFI_CTYPE_INT},
      {"Minute", offsetof(struct mgos_ds3231_date_time, Minute),
       MJS_FFI_CTYPE_INT},
      {"Hour", offsetof(struct mgos_ds3231_date_time, Hour), MJS_FFI_CTYPE_INT},
      {"Dow", offsetof(struct mgos_ds3231_date_time, Dow), MJS_FFI_CTYPE_INT},
      {"Day", offsetof(struct mgos_ds3231_date_time, Day), MJS_FFI_CTYPE_INT},
      {"Month", offsetof(struct mgos_ds3231_date_time, Month),
       MJS_FFI_CTYPE_INT},
      {"Year", offsetof(struct mgos_ds3231_date_time, Year), MJS_FFI_CTYPE_INT},
      {"unixtime", offsetof(struct mgos_ds3231_date_time, unixtime),
       MJS_FFI_CTYPE_INT},
      {NULL, 0, MJS_FFI_CTYPE_NONE},
  };
  return struct_descr;
}

/*
 * Assumes date/time is UTC
 */
static void set_unixtime(struct mgos_ds3231_date_time *date) {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  tm.tm_isdst = -1;
  tm.tm_year = date->Year + 2000 - 1900;
  tm.tm_mon = date->Month - 1;
  tm.tm_mday = date->Day;
  tm.tm_hour = date->Hour;
  tm.tm_min = date->Minute;
  tm.tm_sec = date->Second;
  date->unixtime = mktime(&tm);
}

void mgos_ds3231_date_time_set_time(struct mgos_ds3231_date_time *dt,
                                    uint8_t hour, uint8_t minute,
                                    uint8_t second) {
  dt->Hour = hour;
  dt->Minute = minute;
  dt->Second = second;
  set_unixtime(dt);
}

time_t mgos_ds3231_date_time_get_unixtime(
    const struct mgos_ds3231_date_time *dt) {
  return (NULL != dt) ? dt->unixtime : 0;
}

void mgos_ds3231_date_time_set_unixtime(struct mgos_ds3231_date_time *dt,
                                        time_t unixtime) {
  if (NULL == dt) {
    return;
  }
  struct tm *tm = localtime(&unixtime);
  dt->Year = tm->tm_year - 2000 + 1900;
  dt->Month = tm->tm_mon + 1;
  dt->Day = tm->tm_mday;
  dt->Hour = tm->tm_hour;
  dt->Minute = tm->tm_min;
  dt->Second = tm->tm_sec;
  dt->unixtime = unixtime;
  dt->Dow = dow(tm->tm_year + 1900, dt->Month, dt->Day);
}

uint16_t mgos_ds3231_date_time_get_year(struct mgos_ds3231_date_time *dt) {
  return (NULL != dt) ? (dt->Year + 2000) : 0;
}
/*
 * Registers
 */
#define DS3231_REG_TIME (0x00)
#define DS3231_REG_ALARM_1 (0x07)
#define DS3231_REG_ALARM_2 (0x0B)
#define DS3231_REG_CONTROL (0x0E)
#define DS3231_REG_STATUS (0x0F)
#define DS3231_REG_TEMPERATURE (0x11)

#ifndef _BV
#define _BV(b) (1UL << (b))
#endif

/*
 * utility functions
 *
 * divmod10 from http://forum.arduino.cc/index.php?topic=167414.0
 */
static void divmod10(uint8_t in, uint8_t *div, uint8_t *mod) {
  uint8_t x = (in >> 1);  // div = in/10 <==> div = ((in/2)/5)
  uint8_t q = x + 1;
  x = q;
  q = (q >> 1) + x;
  q = (q >> 3) + x;
  q = (q >> 1) + x;

  uint8_t temp = (q >> 3);
  *mod = in - (((temp << 2) + temp) << 1);
  *div = temp;
}

static uint8_t bin2bcd(uint8_t n) {
  uint8_t div, mod;
  divmod10(n, &div, &mod);
  // return ((n / 10) << 4) + (n % 10);
  return (div << 4) + mod;
}

static uint8_t bcd2bin(uint8_t bcd) {
  // return ((bcd >> 4) * 10) + (bcd % 16);
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

struct mgos_ds3231 {
  uint8_t _addr;
  struct mgos_i2c *_i2c;
};

struct mgos_ds3231 *mgos_ds3231_create(uint8_t addr) {
  // Is I2C enabled?
  if (!mgos_sys_config_get_i2c_enable()) {
    LOG(LL_INFO, ("I2C is disabled."));
    return NULL;
  }

  struct mgos_i2c *i2c = mgos_i2c_get_global();
  if (NULL == i2c) {
    LOG(LL_INFO, ("Could not get i2c global instance"));
    return NULL;
  }

  struct mgos_ds3231 *ds =
      (struct mgos_ds3231 *) calloc(1, sizeof(struct mgos_ds3231));
  if (NULL == ds) {
    LOG(LL_INFO, ("Could not allocate mgos_ds3231 structure."));
    return NULL;
  }

  mgos_i2c_stop(i2c);
  ds->_addr = addr;
  ds->_i2c = i2c;
  // init the DS3231
  // Setup the clock to make sure that it is running, that the oscillator and
  // square wave are disabled, and that alarm interrupts are disabled
  mgos_i2c_write_reg_b(i2c, addr, DS3231_REG_CONTROL, 0x00);
  mgos_ds3231_disable_alarms(ds);
  return ds;
}

void mgos_ds3231_free(struct mgos_ds3231 *ds) {
  if (NULL != ds) {
    mgos_i2c_stop(ds->_i2c);
    free(ds);
  }
}

const struct mgos_ds3231_date_time *mgos_ds3231_read(struct mgos_ds3231 *ds) {
  static struct mgos_ds3231_date_time current_date;
  if (NULL == ds) {
    return &current_date;
  }

  // Read in the 7 bytes which store the
  //  Seconds, Minutes, Hours, Day-Of-Week, Day, Month, Year
  uint8_t data[7];
  if (mgos_i2c_read_reg_n(ds->_i2c, ds->_addr, DS3231_REG_TIME, 7, data)) {
    // current_date.Second = bcd2bin(Wire.read());
    current_date.Second = bcd2bin(data[0]);
    // current_date.Minute = bcd2bin(Wire.read());
    current_date.Minute = bcd2bin(data[1]);

    // 6th Bit of hour indicates 12/24 Hour mode, we will always use 24 hour
    // mode, because we is smart
    uint8_t x = data[2];
    if (x & _BV(6)) {
      current_date.Hour = bcd2bin(x & 0B11111) + (x & _BV(5) ? 0 : 12);
    } else {
      current_date.Hour = bcd2bin(x & 0B111111);
    }

    current_date.Dow = bcd2bin(data[3]);
    current_date.Day = bcd2bin(data[4]);

    x = data[5];
    // bit 7 of month indicates if the year is going to be 100+Year or just Year
    if (x & _BV(7)) {
      current_date.Year = 100;
    } else {
      current_date.Year = 0;
    }
    current_date.Month = bcd2bin(x & 0B01111111);
    current_date.Year += bcd2bin(data[6]);

    set_unixtime(&current_date);
  }
  return &current_date;
}

bool mgos_ds3231_write(struct mgos_ds3231 *ds,
                       const struct mgos_ds3231_date_time *current_date) {
  if (NULL == ds) {
    return false;
  }

  uint8_t data[7];
  data[0] = bin2bcd(current_date->Second);
  data[1] = bin2bcd(current_date->Minute);
  data[2] = bin2bcd(current_date->Hour);
  data[3] =
      bin2bcd(current_date->Dow ? current_date->Dow : 1);  // People might not
                                                           // bother with Dow,
                                                           // make sure it's
                                                           // valid, in case.
  data[4] = bin2bcd(current_date->Day);
  data[5] = bin2bcd(current_date->Month);
  data[6] = bin2bcd(current_date->Year);
  return mgos_i2c_write_reg_n(ds->_i2c, ds->_addr, DS3231_REG_TIME, 7, data);
}

bool mgos_ds3231_write_unixtime(struct mgos_ds3231 *ds, const time_t unixtime) {
  if (NULL == ds) {
    return false;
  }
  struct mgos_ds3231_date_time date;
  mgos_ds3231_date_time_set_unixtime(&date, unixtime);
  return mgos_ds3231_write(ds, &date);
}

int mgos_ds3231_settimeofday(struct mgos_ds3231 *ds) {
  const struct mgos_ds3231_date_time *dt = mgos_ds3231_read(ds);
  time_t now = mgos_ds3231_date_time_get_unixtime(dt);
  return mgos_settimeofday(now, NULL);
}

float mgos_ds3231_get_temperature_c(struct mgos_ds3231 *ds) {
  if (NULL == ds) {
    return 0.0;
  }

  float t = 0;
  uint8_t data[2];
  if (mgos_i2c_read_reg_n(ds->_i2c, ds->_addr, DS3231_REG_TEMPERATURE, 2,
                          data)) {
    int16_t raw = 0x0000;
    if (0x80 & data[0]) {
      // prepare for negative value
      raw = 0xff00;
    }
    raw = (raw | data[0]) << 2;
    raw |= (data[1] >> 6);
    t = raw * 0.25;
  }

  return t;
}

float mgos_ds3231_get_temperature_f(struct mgos_ds3231 *ds) {
  if (NULL == ds) {
    return 0.0;
  }
  float t = mgos_ds3231_get_temperature_c(ds);
  return (t * 1.8f) + 32.0f;
}

bool mgos_ds3231_disable_alarms(struct mgos_ds3231 *ds) {
  if (NULL == ds) {
    return false;
  }
  // There's no way to actually disable the alarms from triggering, so
  // we have to set them to some far future date
  // (NB: you can disable the alarms from putting the SQW pin low, but they
  // still trigger
  //   in the register itself, you can't stop that, hence this tom-foolery).
  const struct mgos_ds3231_date_time *dt = mgos_ds3231_read(ds);
  mgos_ds3231_set_alarm(ds, dt, MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR_DATE);
  mgos_ds3231_set_alarm(ds, dt,
                        MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR_DATE);
  mgos_ds3231_check_alarms(ds);  // A dummy check alarm to kill the alarm flags.
  return true;
}

uint8_t mgos_ds3231_check_alarms(struct mgos_ds3231 *ds) {
  if (NULL == ds) {
    return 0;
  }
  uint8_t status_byte = 0;

  // DS3231_REG_STATUS
  status_byte = mgos_i2c_read_reg_b(ds->_i2c, ds->_addr, DS3231_REG_STATUS);

  uint8_t alarm = status_byte & 0x3;
  if (alarm) {
    // Clear the alarm
    mgos_i2c_write_reg_b(ds->_i2c, ds->_addr, DS3231_REG_STATUS,
                         status_byte & ~0x3);
  }

  return alarm;
}

// MFP mod
uint8_t mgos_ds3231_check_stopflag(struct mgos_ds3231 *ds, int clear) {
  if (NULL == ds) {
    return 0;
  }
  uint8_t status_byte = 0;

  // DS3231_REG_STATUS
  status_byte = mgos_i2c_read_reg_b(ds->_i2c, ds->_addr, DS3231_REG_STATUS);

  uint8_t stopflag = (status_byte & 0x80) && 0x80;
  if (stopflag && clear) {
    // Clear the flag
    mgos_i2c_write_reg_b(ds->_i2c, ds->_addr, DS3231_REG_STATUS,
                         status_byte & 0x7F);
  }

  return stopflag;
}

uint8_t mgos_ds3231_set_alarm(struct mgos_ds3231 *ds,
                              const struct mgos_ds3231_date_time *alarm_date,
                              uint8_t alarm_mode) {
  if (NULL == ds) {
    return 0;
  }
  // Read the control byte, we will need to modify the alarm enable bits
  uint8_t control_byte =
      mgos_i2c_read_reg_b(ds->_i2c, ds->_addr, DS3231_REG_CONTROL);

  if ((alarm_mode & 0B00000011) == 0B00000011) {
    // Set to the equivalent Alarm Mode 2,
    //    for Hourly this will be Match Minute,
    //    for Daily it will be Match Minute and Match Hour,
    //    for Weekly, Match Minute, Hour and Day-Of-Week (Dow)
    //    for Montly, Match Minute, Hour, and Day-Of-Month (Date)

    alarm_mode = alarm_mode & ~0x01;
  }

  uint8_t data[8] = {};
  size_t i = 0;
  if (alarm_mode & 0B00000001) /* Alarm 1 Modes */ {
    data[i++] = DS3231_REG_ALARM_1;
    data[i++] = bin2bcd(alarm_date->Second) | (alarm_mode & 0B10000000);
    control_byte = control_byte | _BV(0) |
                   _BV(2);  // Enable Alarm 1, set interrupt output on alarm.
  } else {
    // Start address of data for Alarm2
    data[i++] = DS3231_REG_ALARM_2;
    control_byte = control_byte | _BV(1) |
                   _BV(2);  // Enable Alarm 2, set interrupt output on alarm.
  }
  alarm_mode = alarm_mode << 1;

  data[i++] = bin2bcd(alarm_date->Minute) | (alarm_mode & 0B10000000);
  alarm_mode = alarm_mode << 1;
  data[i++] = bin2bcd(alarm_date->Hour) | (alarm_mode & 0B10000000);
  alarm_mode = alarm_mode << 1;

  if (alarm_mode & 0B01000000) {  // DOW indicator
    data[i++] = bin2bcd(alarm_date->Dow) | (alarm_mode & 0B10000000) | _BV(6);
  } else {
    data[i++] = bin2bcd(alarm_date->Day) | (alarm_mode & 0B10000000);
  }
  alarm_mode = alarm_mode << 2;  // Value and Date/Day indicator

  mgos_i2c_write(ds->_i2c, ds->_addr, data, i, true);

  // Write the control byte
  mgos_i2c_write_reg_b(ds->_i2c, ds->_addr, DS3231_REG_CONTROL, control_byte);

  return alarm_mode >> 5;
}

static const uint8_t *get_alarm_regs(struct mgos_ds3231 *ds, uint8_t alarm) {
  static uint8_t buf[4];
  size_t count = (1 == alarm) ? 4 : 3;
  uint8_t reg = (1 == alarm) ? DS3231_REG_ALARM_1 : DS3231_REG_ALARM_2;
  mgos_i2c_read_reg_n(ds->_i2c, ds->_addr, reg, count, buf);
  return buf;
}

const uint8_t *mgos_ds3231_get_alarm1(struct mgos_ds3231 *ds) {
  return get_alarm_regs(ds, 1);
}

const uint8_t *mgos_ds3231_get_alarm2(struct mgos_ds3231 *ds) {
  return get_alarm_regs(ds, 2);
}

bool mgos_ds3231_init() {
  return true;
}
