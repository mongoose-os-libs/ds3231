# A native Mongoose OS DS3231 RTC library

## Implementation
The library implements 2 structures: `struct mgos_ds3231` and a helper `struct mgos_ds3231_date_time`.
Both structures are available for mJS too.

### struct mgos_ds3231
This is used to communicate with the DS3231.
The RTC data is read into and written from using the helper `struct mgos_ds3231_date_time`

#### Example in C
```c
struct mgos_ds3231* ds=mgos_ds3231_create(addr);
time_t unixtime=time(NULL);
mgos_ds3231_write_unixtime(ds, unixtime);
mgos_ds3231_free(ds);
```

#### Example in mJS
```javascript
let ds=DS3231.create(addr);
let unixtime=Timer.now();
ds.writeUnixtime(unixtime);
ds.free();
```

### struct mgos_ds3231_date_time
Encapsulates the DS3231 date/time information plus the unix timestamp.
Several functions are provided to create and free the structure and to get/set different fields.

#### Example in C
```c
struct mgos_ds3231_date_time* dt=mgos_ds3231_date_time_create();
mgos_ds3231_date_time_set_date(dt, 2016, 2, 3);
// mgos_ds3231_date_time_set_time will calculate the unixtime.
// It is VERY important to call mgos_ds3231_date_time_set_date first
mgos_ds3231_date_time_set_time(dt, 12, 34, 56);

struct mgos_ds3231* ds=mgos_ds3231_create(addr);
mgos_ds3231_write(ds, dt);
mgos_ds3231_settimeofday(ds);

mgos_ds3231_free(ds);
mgos_ds3231_date_time_free(dt);
```

#### Example in mJS
```javascript
let dt=DS3231DateTime.create();
dt.setDate(2016, 2, 3);
dt.setTime(12, 34, 56);

let ds=DS3231.create(addr);
ds.write(dt);
ds.setTimeOfDay();

ds.free();
dt.free();
```
