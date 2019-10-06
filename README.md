# Zabbix Redis exporter

A Zabbix module to export "raw" history data to Redis to be futher processed by other applications. For example the [zabbix_redis_influxdb](https://github.com/2kgwf/zabbix_redis_influxdb) which resolves the item names, keys, hosts, etc and stores that data into InfluxDB.

NOTE: This is somewhat early in the development, and thus not yet recommended to be used in critical production systems.

## Building

### Dependencies

Preferably clone this repository and both of these into "adjacent directories"

- [Zabbix sources](https://git.zabbix.com/projects/ZBX/repos/zabbix/browse)
- [Hiredis](https://github.com/redis/hiredis)

### Compilation

First make sure you have built the `hiredis` lib (should be just running `make` there)

To compile `zabbix_redis_exporter`, just run `make`.

If you have zabbix sources and/or hiredis located elsewhere than `../zabbix` and `../hiredis`, specify their path with `ZABBIX_SRC_DIR` and `HIREDIS_SRC_DIR` environment variables, for example:

```sh
ZABBIX_SRC_DIR=/path/to/zabbix HIREDIS_SRC_DIR=/path/to/hiredis make
```

## Usage

Add the built `zabbix_redis_exporter` module to `zabbix_server.conf` in a `LoadModule` property, for example: `LoadModule=/usr/lib/zabbix/zabbix_redis_exporter.so`

If you have redis running on a different host or in a non-default port, create a config file `/etc/zabbix/zabbix_redis_exporter.conf` with the host and port:

```
RedisHost=host.or.ip.address
RedisPort=1234
```

If all is well, after (re)starting Zabbix server, the history syncer processes will push history data to Redis in the end of history sync procedure after data is written into Zabbix database and saved in value cache.

## Data format

zabbix_redis_exporter will do a `RPUSH zabbix_history` with the history data (so that you can easily `LPOP zabbix_history` the data in your "consumer application").

The value is a space-delimetered string with the following format:

`<timestamp seconds> <nanoseconds> <item id> <item type> <item value>`

Where item type is a char defining the type:

- `f` - float
- `i` - integer
- `s` - string
- `t` - text
- `l` - log

Example: Item ID: `23273` which is a float, with a value of `3.605175` at 2019-10-03 5:14pm (UTC):

`1570122893 539748029 23273 f 3.605175`
