STAT_WGET := $(shell command -v wget 2> /dev/null)
STAT_CURL := $(shell command -v curl 2> /dev/null)
ifneq ("$(wildcard $(NEXMON_ROOT)/.UUID)","")
STAT_UUID := $(shell cat $(NEXMON_ROOT)/.UUID | base64)
else
STAT_UUID := $(shell cat /dev/urandom | LC_ALL=C tr -dc A-Z0-9 | head -c32 > $(NEXMON_ROOT)/.UUID && cat $(NEXMON_ROOT)/.UUID | base64)
endif
STAT_UNAME := $(shell uname -srmp | base64)
STAT_PATH := $(shell git rev-parse --show-prefix | base64)
ifeq ("$(STAT_PATH)","Cg==")
STAT_PATH := $(shell echo $$(cd .. && git rev-parse --show-prefix)$$(basename `pwd`) | base64)
endif
STAT_GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always --tags | base64)
STAT_GIT_REMOTE := $(shell git config --get remote.origin.url | base64)
#STAT_URL := http://172.16.121.1:8888/statistics/
STAT_URL := https://nexmon.org/statistics/
STAT_DATA := "uuid=$(STAT_UUID)&uname=$(STAT_UNAME)&path=$(STAT_PATH)&version=$(STAT_GIT_VERSION)&remote=$(STAT_GIT_REMOTE)"

statistics: FORCE
ifdef STAT_WGET
	$(Q)$(STAT_WGET) --user-agent="Nexmon" --post-data=$(STAT_DATA) --quiet --background --delete-after --no-check-certificate $(STAT_URL) > /dev/null 2> /dev/null
else
ifdef STAT_CURL
	$(Q)$(STAT_CURL) -A "Nexmon" --data $(STAT_DATA) $(STAT_URL) > /dev/null 2> /dev/null &
endif
endif

FORCE:
