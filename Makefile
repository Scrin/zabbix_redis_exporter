# You can set ZABBIX_SRC_DIR and HIREDIS_SRC_DIR environment variables if they are cloned elsewhere
# For example, call make with:
# ZABBIX_SRC_DIR=/path/to/somewhere HIREDIS_SRC_DIR=/path/to/elsewhere make
# Debug logging to /tmp/zabbix_redis_exporter.log can be similarly enabled by setting ENABLE_DEBUG_LOG=1

ifndef ZABBIX_SRC_DIR
ZABBIX_SRC_DIR=../zabbix
endif
ifndef HIREDIS_SRC_DIR
HIREDIS_SRC_DIR=../hiredis
endif
ifndef ENABLE_DEBUG_LOG
ENABLE_DEBUG_LOG=0
endif

CC=gcc
CFLAGS=-Wall -fPIC -shared -DENABLE_DEBUG_LOG=${ENABLE_DEBUG_LOG}
LDFLAGS=-Wl,-Bstatic -lhiredis -Wl,-Bdynamic
INC=-I${ZABBIX_SRC_DIR}/include -I${HIREDIS_SRC_DIR}
LIBS=-L${HIREDIS_SRC_DIR}
SOURCES=zabbix_redis_exporter.c
OUTPUT=zabbix_redis_exporter.so

all: $(SOURCES)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCES) $(INC) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)
