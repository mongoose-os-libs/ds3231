#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Alarms
 */
/*
 * Alarm bits
 * High nibble is mask : M1 M2 M3 M4
 * Low nibble is 1 for alarm 1, 2 for alarm2, 3 for custom alarm
 */
/* Alarm 1 */
static const uint8_t MGOS_DS3231_ALARM_EVERY_SECOND = 0B11110001;
static const uint8_t MGOS_DS3231_ALARM_MATCH_SECOND = 0B01110001;
static const uint8_t MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE = 0B00110001;
static const uint8_t MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR = 0B00010001;
static const uint8_t MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR_DATE = 0B00000001;
static const uint8_t MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR_DOW = 0B00001001;

/* Alarm 2 */
static const uint8_t MGOS_DS3231_ALARM_EVERY_MINUTE = 0B01110010;
static const uint8_t MGOS_DS3231_ALARM_MATCH_MINUTE = 0B00110010;
static const uint8_t MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR = 0B00010010;
static const uint8_t MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR_DATE = 0B00000010;
static const uint8_t MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR_DOW = 0B00001010;

/* Custom alarm*/
static const uint8_t MGOS_DS3231_ALARM_HOURLY = 0B00110011;
static const uint8_t MGOS_DS3231_ALARM_DAILY = 0B00010011;
static const uint8_t MGOS_DS3231_ALARM_WEEKLY = 0B00001011;
static const uint8_t MGOS_DS3231_ALARM_MONTHLY = 0B00000011;

struct mgos_ds3231;

/*
 * struct mgos_ds3231_date_time begin
 */
struct mgos_ds3231_date_time {
  uint32_t Second; // 0-59
  uint32_t Minute; // 0-59
  uint32_t Hour; // 0-23
  uint32_t Dow; // 1-7 (Day Of Week)
  uint32_t Day; // 1-31
  uint32_t Month; // 1-12
  uint32_t Year; // 0-199
  time_t unixtime;
};

struct mjs_c_struct_member;

/*
 * Create a `mgos_ds3231_date_time` structure
 */
struct mgos_ds3231_date_time* mgos_ds3231_date_time_create();

/*
 * Free the `mgos_ds3231_date_time` structure
 */
void mgos_ds3231_date_time_free(struct mgos_ds3231_date_time* dt);

/*
 * Set the date part of the `mgos_ds3231_date_time` structure.
 * The `Dow` member is computed by the function
 */
void mgos_ds3231_date_time_set_date(struct mgos_ds3231_date_time* dt, uint16_t year, uint8_t month, uint8_t day);

/*
 * Set the time part of the `mgos_ds3231_date_time` structure.
 * The `unixtime` member is set by this function. The day/time MUST be UTC
 * `mgos_ds3231_date_time_set_date` should be called before.
 */
void mgos_ds3231_date_time_set_time(struct mgos_ds3231_date_time* dt, uint8_t hour, uint8_t minute, uint8_t second);

/*
 * Get the structure description
 */
const struct mjs_c_struct_member* mgos_ds3231_date_time_get_struct_descr();

/*
 * Get the `unixtime` time part of the `mgos_ds3231_date_time` structure.
 */
time_t mgos_ds3231_date_time_get_unixtime(const struct mgos_ds3231_date_time* dt);

/*
 * Set the members of `struct mgos_ds3231_date_time` from the provided `unixtime`
 */
void mgos_ds3231_date_time_set_unixtime(struct mgos_ds3231_date_time* dt, time_t unixtime);

/*
 *
 */
uint16_t mgos_ds3231_date_time_get_year(struct mgos_ds3231_date_time* dt);

/*
 * struct mgos_ds3231_date_time end
 */


/*
 * Create a `struct mgos_ds3231` structure
 */
struct mgos_ds3231* mgos_ds3231_create(uint8_t addr);

/*
 * Free a `struct mgos_ds3231` structure
 */
void mgos_ds3231_free(struct mgos_ds3231* ds);

/*
 * Read the current date and time, returning a structure containing that information.
 */
const struct mgos_ds3231_date_time* mgos_ds3231_read(struct mgos_ds3231* ds);

/*
 * Set the date and time from the settings in the given structure.
 */
bool mgos_ds3231_write(struct mgos_ds3231* ds, const struct mgos_ds3231_date_time* date);

/*
 * Set the date and time from unixtime.
 */
bool mgos_ds3231_write_unixtime(struct mgos_ds3231* ds, const time_t unixtime);

/*
 * Sets the system time from the DS3231 data.
 * Assumes DS3231 was previously setup with correct data.
 *
 * Returns 0 if success.
 */
int mgos_ds3231_settimeofday(struct mgos_ds3231* ds);

/*
 * Get the temperature accurate to within 0.25 Celsius
 */
float mgos_ds3231_get_temperature_c(struct mgos_ds3231* ds);

/*
 * Get the temperature in Fahrenheit
 */
float mgos_ds3231_get_temperature_f(struct mgos_ds3231* ds);

/** Disable any existing alarm settings.
 *
 *  @return Success True/False
 */
bool mgos_ds3231_disable_alarms(struct mgos_ds3231* ds);

/*
 * Determine if an alarm has triggered, also clears the alarm if so.
 *
 *  Returns 0 for no alarm, 1 for Alarm 1, 2 for Alarm 2, and 3 for both alarms
 */
uint8_t mgos_ds3231_check_alarms(struct mgos_ds3231* ds);

// MFP mod
/*
 * Determine if the oscillator stop flag is set
 *
 *  Returns the flag status
 */
uint8_t mgos_ds3231_check_stopflag(struct mgos_ds3231* ds, int clear);

/*
 * Sets an alarm, the alarm will pull the SQW pin low (you can monitor with an interrupt).
 *
 *  `alarm_date` The date/time for the alarm, as appropriate for the alarm mode (example, for
 *    ALARM_MATCH_SECOND then alarm_date.Second will be the matching criteria).
 *
 *  `alarm_mode` the mode of the alarm, from the following...
 *
 *    MGOS_DS3231_ALARM_EVERY_SECOND
 *    MGOS_DS3231_ALARM_MATCH_SECOND
 *    MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE
 *    MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR
 *    MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR_DATE
 *    MGOS_DS3231_ALARM_MATCH_SECOND_MINUTE_HOUR_DOW
 *
 *    MGOS_DS3231_ALARM_EVERY_MINUTE
 *    MGOS_DS3231_ALARM_MATCH_MINUTE
 *    MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR
 *    MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR_DATE
 *    MGOS_DS3231_ALARM_MATCH_MINUTE_HOUR_DOW
 *
 *    MGOS_DS3231_ALARM_HOURLY
 *    MGOS_DS3231_ALARM_DAILY
 *    MGOS_DS3231_ALARM_WEEKLY
 *    MGOS_DS3231_ALARM_MONTHLY
 *
 */
uint8_t mgos_ds3231_set_alarm(struct mgos_ds3231* ds, const struct mgos_ds3231_date_time* alarm_date, uint8_t alarm_mode);

const uint8_t* mgos_ds3231_get_alarm1(struct mgos_ds3231* ds);
const uint8_t* mgos_ds3231_get_alarm2(struct mgos_ds3231* ds);

#ifdef __cplusplus
}
#endif
