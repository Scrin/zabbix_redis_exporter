#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "module.h"
#include "hiredis.h"

#define CONFIG_FILE "/etc/zabbix/zabbix_redis_exporter.conf"
#define SEND_DATA_MAX_SIZE 1024 * 1024

static redisContext *redis = NULL;
static char redis_host[256] = "127.0.0.1";
static int redis_port = 6379;

static void debug_log(const char *msg)
{
#if ENABLE_DEBUG_LOG == 1
	static long long log_counter = 0;
	FILE *f = fopen("/tmp/zabbix_redis_exporter.log", "a");
	fprintf(f, "%lld: %s\n", log_counter++, msg);
	fclose(f);
#endif
}

static void get_conf(const char *line, const char *prop, char *target)
{
	int len = strlen(prop);
	if (strncmp(prop, line, len) == 0)
	{
		strncpy(target, strchr(line, '=') + 1, strchr(line, '\n') - line - len);
	}
}

int zbx_module_init(void)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(CONFIG_FILE, "r");
	if (fp == NULL)
	{
		debug_log("Config file missing");
		return ZBX_MODULE_OK;
	}

	char port_str[16] = "";
	while ((read = getline(&line, &len, fp)) != -1)
	{
		get_conf(line, "RedisHost=", redis_host);
		get_conf(line, "RedisPort=", port_str);
	}
	if (strlen(port_str) > 0)
	{
		redis_port = atoi(port_str);
	}
	debug_log(redis_host);
	debug_log(port_str);

	fclose(fp);
	if (line)
	{
		free(line);
	}

	return ZBX_MODULE_OK;
}

static void redis_connect()
{
	struct timeval timeout = {1, 500000}; // 1.5 seconds
	redis = redisConnectWithTimeout(redis_host, redis_port, timeout);
	if (redis == NULL || redis->err)
	{
		if (redis)
		{
			debug_log(redis->errstr);
		}
		else
		{
			debug_log("Can't allocate redis context");
		}
	}
	else
	{
		debug_log("Connected");
	}
}

static void redis_reconnect()
{
	debug_log("Reconnecting");
	redisFree(redis);
	redis_connect();
}

static void send(const char *data)
{
	if (redis == NULL)
	{
		redis_connect();
	}
	debug_log(data);

	redisReply *reply = redisCommand(redis, "RPUSH zabbix_history %s", data);
	if (reply != NULL)
	{
		freeReplyObject(reply);
		return;
	}

	debug_log(redis->errstr);
	redis_reconnect();

	reply = redisCommand(redis, "RPUSH zabbix_history %s", data);
	if (reply != NULL)
	{
		freeReplyObject(reply);
		return;
	}

	debug_log(redis->errstr);
}

static void h_float(const ZBX_HISTORY_FLOAT *history, int count)
{
	for (int i = 0; i < count; i++)
	{
		char buffer[SEND_DATA_MAX_SIZE];
		int len = snprintf(buffer, SEND_DATA_MAX_SIZE, "%d %d %lu f %f", history[i].clock, history[i].ns, history[i].itemid, history[i].value);
		if (len < 0 || len >= SEND_DATA_MAX_SIZE)
		{
			debug_log("Truncated data!");
		}
		send(buffer);
	}
}

static void h_integer(const ZBX_HISTORY_INTEGER *history, int count)
{
	for (int i = 0; i < count; i++)
	{
		char buffer[SEND_DATA_MAX_SIZE];
		int len = snprintf(buffer, SEND_DATA_MAX_SIZE, "%d %d %lu i %lu", history[i].clock, history[i].ns, history[i].itemid, history[i].value);
		if (len < 0 || len >= SEND_DATA_MAX_SIZE)
		{
			debug_log("Truncated data!");
		}
		send(buffer);
	}
}

static void h_string(const ZBX_HISTORY_STRING *history, int count)
{
	for (int i = 0; i < count; i++)
	{
		char buffer[SEND_DATA_MAX_SIZE];
		int len = snprintf(buffer, SEND_DATA_MAX_SIZE, "%d %d %lu s %s", history[i].clock, history[i].ns, history[i].itemid, history[i].value);
		if (len < 0 || len >= SEND_DATA_MAX_SIZE)
		{
			debug_log("Truncated data!");
		}
		send(buffer);
	}
}

static void h_text(const ZBX_HISTORY_TEXT *history, int count)
{
	for (int i = 0; i < count; i++)
	{
		char buffer[SEND_DATA_MAX_SIZE];
		int len = snprintf(buffer, SEND_DATA_MAX_SIZE, "%d %d %lu t %s", history[i].clock, history[i].ns, history[i].itemid, history[i].value);
		if (len < 0 || len >= SEND_DATA_MAX_SIZE)
		{
			debug_log("Truncated data!");
		}
		send(buffer);
	}
}

static void h_log(const ZBX_HISTORY_LOG *history, int count)
{
	for (int i = 0; i < count; i++)
	{
		char buffer[SEND_DATA_MAX_SIZE];
		int len = snprintf(buffer, SEND_DATA_MAX_SIZE, "%d %d %lu l %d %d %d %s %s", history[i].clock, history[i].ns, history[i].itemid,
						   history[i].timestamp, history[i].logeventid, history[i].severity, history[i].source, history[i].value);
		if (len < 0 || len >= SEND_DATA_MAX_SIZE)
		{
			debug_log("Truncated data!");
		}
		send(buffer);
	}
}

int zbx_module_api_version(void)
{
	return ZBX_MODULE_API_VERSION;
}

ZBX_HISTORY_WRITE_CBS zbx_module_history_write_cbs(void)
{
	static ZBX_HISTORY_WRITE_CBS cbs = {h_float, h_integer, h_string, h_text, h_log};
	return cbs;
}
