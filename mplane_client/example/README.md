# mpclient-demo
This "demo client" is a program that can send arbitrary commands to the server.
It is designed with several applications in mind:
* Showing visually how the M-Plane works
* Providing a testbed to M-Plane developers to validate server implementations
* Integration into shell scripts

This client is minimal. It ingests a file containing JSON commands whose schema
follows the `CommandSequence` protobuf, a wrapper of a repeated `oneof` which
can hold any of the `Request` types. The client sequentially reads each request
and executes the corresponding mpclient RPC.

## Usage
```
./wrapper.sh ./mpclient-demo \
  --commands ../example/cases/connect.json \
  --netconfHost 192.168.10.13 \
  --netconfUser oran \
  --netconfPassword Password123 \
  --insecure
```

Note that there must be a server instance running. If 0 commands are found,
there may be a syntax or schema error in the commands file.

When mplane_server is running on the same host:
```bash
# Start the server (mpc_client):
./wrapper.sh ./mpc_client --insecure

# Separately, run the client (mpclient-demo):
./wrapper.sh ./mpclient-demo \
  --commands ../example/cases/connect.json \
  --netconfHost 127.0.0.1 \
  --netconfPort 830 \
  --netconfUser root \
  --netconfPassword facebook \
  --insecure
```
Using mpclient-demo for the Dockerized build on Linux requires updating the compose file to add the host-gateway 
for the mplane-client-tester or mplane-client-integrated-tester service, and setting host.docker.internal as netconfHost:
```
    extra_hosts:
      - "host.docker.internal:host-gateway"
```

### Commands File Example
```json
{
  "commands": [
    {
      "connect": {}
    },
    {
      "netconfRpc": {
        "serializedYang": "<get-config xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><source><running/></source></get-config>",
        "timeoutSec": 3
      }
    },
    {
      "netconfRpc": {
        "serializedYang": "<create-subscription xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\"><stream>o-ran-supervision</stream></create-subscription>",
        "timeoutSec": 3
      }
    },
    {
      "streamNotifications": {}
    }
  ]
}
```

Notes:
- Refer to the parent [README.md](../README.md) for a list of available
  commands.
- `netconfRpc` is synchronous and prints its output before the next command
  executes.
- `streamNotifications` is asynchronous and prints output in blocks whenever a
  new notification arrives.
- It is not necessary to populate NETCONF connection parameters (e.g. `host`,
  `user`, etc.) as long as they are supplied as command line arguments.
  `sessionId` is dynamically filled in once a connection is established via
  connect or listen.
