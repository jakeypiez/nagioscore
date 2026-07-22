FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        apache2 \
        apache2-utils \
        autoconf \
        build-essential \
        ca-certificates \
        curl \
        iputils-ping \
        libapache2-mod-php \
        libgd-dev \
        libssl-dev \
        monitoring-plugins-basic \
        php \
        procps \
        unzip \
    && rm -rf /var/lib/apt/lists/*

# Nagios regenerates the worker-prescheduling perfect hash during source builds.
RUN apt-get update \
    && apt-get install -y --no-install-recommends gperf \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --system nagios \
    && useradd --system --gid nagios --home-dir /usr/local/nagios --shell /usr/sbin/nologin nagios \
    && usermod --append --groups nagios www-data

WORKDIR /build/nagioscore
COPY . .

RUN ./configure \
        --prefix=/usr/local/nagios \
        --with-command-group=nagios \
        --with-httpd-conf=/etc/apache2/conf-available \
        --with-mail=/bin/true \
        --with-nagios-group=nagios \
        --with-nagios-user=nagios \
    && make -j"$(nproc)" all \
    && make install \
    && make install-config \
    && make install-commandmode \
    && make install-webconf \
    && cp -a /usr/lib/nagios/plugins/. /usr/local/nagios/libexec/ \
    && chown -R nagios:nagios /usr/local/nagios/var \
    && a2enmod cgi headers \
    && a2enconf nagios \
    && rm -rf /build/nagioscore

COPY --chown=root:nagios docker/lab/lab.cfg /usr/local/nagios/etc/objects/lab.cfg
RUN printf '\ncfg_file=/usr/local/nagios/etc/objects/lab.cfg\n' >> /usr/local/nagios/etc/nagios.cfg

COPY docker/lab/entrypoint.sh /usr/local/bin/nagios-lab-entrypoint
RUN chmod 0755 /usr/local/bin/nagios-lab-entrypoint

EXPOSE 80
ENTRYPOINT ["/usr/local/bin/nagios-lab-entrypoint"]
