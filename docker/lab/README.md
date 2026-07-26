# Browser test lab

This lab builds the current checkout of Nagios Core, including the modern web
frontend, and exposes it only on the Mac's detected RFC1918 LAN address. It is
intended for hands-on browser testing on a trusted home or office network.

## Start it

Docker Desktop must be running. From the repository root:

```sh
./scripts/nagios-lab up
```

The script prints the exact URL and a randomly generated test password. The
credentials live only in `.nagios-lab/`, are mode `0700`/`0600`, and are
excluded from both Git and the Docker build context. The Compose service passes
them to the container as Docker secrets rather than environment variables.

The default address on this Mac is expected to resemble:

```text
http://192.168.x.x:8080/nagios/
```

Open that address from any browser on the same private network. If macOS asks
whether Docker may accept incoming connections, allow it for the private
network. A custom private address or port can be selected when needed:

```sh
NAGIOS_LAB_BIND_IP=192.168.1.50 NAGIOS_LAB_PORT=9080 ./scripts/nagios-lab up
```

## Useful commands

```sh
./scripts/nagios-lab status
./scripts/nagios-lab credentials
./scripts/nagios-lab logs
./scripts/nagios-lab down
```

The lab includes three deterministic demo hosts and seven demo services in
healthy, warning, and critical states. Those warning and critical results are
intentional: they make the problem views, filtering, acknowledgements, comments,
and scheduled-downtime flows available for manual testing. Notifications are
disabled for every demo object.

## Security boundary

The launcher refuses loopback, wildcard, public, and non-RFC1918 bind addresses;
the container has `no-new-privileges` enabled and does not restart on its own.
Apache uses bcrypt-protected Basic Authentication and the frontend's security
headers, but the LAN connection itself is plain HTTP. Treat this as a disposable
trusted-LAN lab: do not port-forward it, expose it to the internet, or reuse its
password for anything else. Production deployments should terminate TLS at
Apache or a trusted reverse proxy.
