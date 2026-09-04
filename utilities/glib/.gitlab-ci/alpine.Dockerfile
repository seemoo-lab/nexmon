FROM alpine:3.19

RUN apk add --no-cache \
    bash \
    build-base \
    bzip2-dev \
    curl \
    dbus \
    diffutils \
    desktop-file-utils \
    docbook-xml \
    docbook-xsl \
    gettext-dev \
    git \
    libffi-dev \
    libxml2-utils \
    libxslt \
    meson \
    musl-locales \
    py3-pip \
    python3 \
    pcre2-dev \
    shared-mime-info \
    tzdata \
    util-linux-dev \
    zlib-dev

ENV LANG=C.UTF-8 LANGUAGE=C.UTF-8 LC_ALL=C.UTF-8 MUSL_LOCPATH=/usr/share/i18n/locales/musl

RUN pip3 install --break-system-packages meson==1.4.2
RUN pip3 install --break-system-packages python-gitlab==6.5.0

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN adduser -D -u $HOST_USER_ID -s /bin/bash user

USER user
WORKDIR /home/user

COPY cache-subprojects.sh .
RUN ./cache-subprojects.sh
