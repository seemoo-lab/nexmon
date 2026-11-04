ifneq ("$(wildcard $(NEXMON_ROOT)/DISABLE_STATISTICS)","")
# Telemetry disabled via DISABLE_STATISTICS opt-out marker
statistics: FORCE
	@true
else
STAT_WGET := $(shell command -v wget 2> /dev/null)
STAT_CURL := $(shell command -v curl 2> /dev/null)
ifneq ("$(wildcard $(NEXMON_ROOT)/.UUID)","")
STAT_UUID := $(shell cat $(NEXMON_ROOT)/.UUID | base64 | tr -d '\r\n')
else
STAT_UUID := $(shell cat /dev/urandom | LC_ALL=C tr -dc A-Z0-9 | head -c32 > $(NEXMON_ROOT)/.UUID && cat $(NEXMON_ROOT)/.UUID | base64 | tr -d '\r\n')
endif
STAT_UNAME := $(shell uname -srmp | base64 | tr -d '\r\n')
STAT_PATH := $(shell git rev-parse --show-prefix | base64 | tr -d '\r\n')
ifeq ("$(STAT_PATH)","Cg==")
STAT_PATH := $(shell echo $$(cd .. && git rev-parse --show-prefix)$$(basename `pwd`) | base64 | tr -d '\r\n')
endif
STAT_GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always --tags | base64 | tr -d '\r\n')
STAT_GIT_REMOTE := $(shell git config --get remote.origin.url 2>/dev/null | base64 | tr -d '\r\n')
#STAT_URL := http://172.16.121.1:8888/statistics/
STAT_URL := https://nexmon.org/statistics/
STAT_DATA := "uuid=$(STAT_UUID)&uname=$(STAT_UNAME)&path=$(STAT_PATH)&version=$(STAT_GIT_VERSION)&remote=$(STAT_GIT_REMOTE)"

statistics: FORCE
ifdef STAT_WGET
	$(Q)$(STAT_WGET) --user-agent="Nexmon" --post-data=$(STAT_DATA) --quiet --background --delete-after $(STAT_URL) > /dev/null 2> /dev/null
else
ifdef STAT_CURL
	$(Q)$(STAT_CURL) -A "Nexmon" --data $(STAT_DATA) $(STAT_URL) > /dev/null 2> /dev/null &
endif
endif
endif

FORCE:
