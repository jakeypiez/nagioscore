#!/usr/bin/env bash
set -Eeuo pipefail

readonly username_file=/run/secrets/lab_username
readonly password_file=/run/secrets/lab_password
readonly htpasswd_file=/usr/local/nagios/etc/htpasswd.users
readonly cgi_config=/usr/local/nagios/etc/cgi.cfg
readonly nagios_config=/usr/local/nagios/etc/nagios.cfg

for secret_file in "$username_file" "$password_file"; do
    if [[ ! -s "$secret_file" ]]; then
        printf 'Required Docker secret is missing: %s\n' "$secret_file" >&2
        exit 1
    fi
done

username=$(tr -d '\r\n' < "$username_file")
password=$(tr -d '\r\n' < "$password_file")

if [[ ! "$username" =~ ^[A-Za-z0-9._-]{3,64}$ ]]; then
    printf 'The lab username contains unsupported characters.\n' >&2
    exit 1
fi

if (( ${#password} < 16 )); then
    printf 'The lab password must contain at least 16 characters.\n' >&2
    exit 1
fi

umask 0027
printf '%s\n' "$password" | htpasswd -ciB "$htpasswd_file" "$username" >/dev/null
chown root:www-data "$htpasswd_file"
chmod 0640 "$htpasswd_file"

# Keep every privileged CGI action restricted to the generated lab user.
sed -i -E "s/^(authorized_for_[A-Za-z_]+=).*/\\1${username}/" "$cgi_config"

mkdir -p /run/apache2 /usr/local/nagios/var/rw /usr/local/nagios/var/spool/checkresults
chown -R nagios:nagios /usr/local/nagios/var
chmod 2775 /usr/local/nagios/var/rw /usr/local/nagios/var/spool/checkresults

runuser --user nagios -- /usr/local/nagios/bin/nagios -v "$nagios_config"

nagios_pid=''
apache_pid=''

shutdown() {
    trap - INT TERM EXIT
    [[ -z "$apache_pid" ]] || kill -TERM "$apache_pid" 2>/dev/null || true
    [[ -z "$nagios_pid" ]] || kill -TERM "$nagios_pid" 2>/dev/null || true
    [[ -z "$apache_pid" ]] || wait "$apache_pid" 2>/dev/null || true
    [[ -z "$nagios_pid" ]] || wait "$nagios_pid" 2>/dev/null || true
}

trap shutdown INT TERM EXIT

runuser --user nagios -- /usr/local/nagios/bin/nagios "$nagios_config" &
nagios_pid=$!
apache2ctl -D FOREGROUND &
apache_pid=$!

wait -n "$nagios_pid" "$apache_pid"
