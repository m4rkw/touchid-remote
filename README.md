# TouchID Remote - use touchID over SSH!

This project builds on the awesome [touch2sudo](https://github.com/prbinu/touch2sudo) and allows it to be used with
remote ssh connections. This means you can use the touchID sensor on your Mac to authenticate sudo when connected to remote machines over ssh.


## Overview

There are two components:

1) A PAM module which must be installed on the remote server ([pam\_tid.so](https://github.com/m4rkw/touchid-remote/blob/master/pam_tid.c))

2) A python authentication daemon which runs locally and invokes touch2sudo
([server.py](https://github.com/m4rkw/touchid-remote/blob/master/server.py))

The basic process is:

1) When the user connects to the remote machine via ssh, a tunnel is created
back to the authentication server on the client machine.

2) When the user invokes sudo the PAM module executes and checks whether the
authentication service is available and if it's allowed to be used with the
current user. If these conditions are met and it can connect to the
authentication service through the remote tunnel it will pass a static key to
the authentication server.

3) The authentication server on the client machine validates the shared secret,
if it's invalid then it returns '0' and the PAM module returns an error status
which then causes PAM to defer to the next authentication mechanism which would
be a password prompt by default.

4) If the shared secret is correct the authentication server invokes touch2sudo
which prompts the user to authenticate with TouchID. If this is successful this
is indicated back to the remote machine with a second shared secret.

5) The PAM module validates the second shared secret and if this is correct then
the user is authenticated and the sudo command completes.

Additionally the USE\_TOUCHID environment variable must be set to '1' in order
for TouchID to be used. This is so that in a scenario where you have a mac
connected (so the tunnel is available) but you're connecting on a different
device that doesn't have a TouchID authentication server (eg an iPhone) you can
still authenticate normally.

## Installation

1) Compile and install the PAM module on your remote servers:

````
$ sudo apt install libconfig-dev
$ git clone https://github.com/m4rkw/touchid-remote
$ cd touchid-remote
$ make
$ sudo make install
````

2) Grab touch2sudo from: https://github.com/prbinu/touch2sudo/releases and
install it somewhere in your PATH, eg /usr/local/bin/touch2sudo

3) Install the authentication server on your mac terminal:

````
$ git clone https://github.com/m4rkw/touchid-remote
$ make install
````

4) Edit ~/.touchid-remote/touchid-remote.conf and set key1 and key2 to long
random strings (max 128 chars). Also edit /etc/touchid-remote.conf on the remote
machine(s) and set key1 and key2 to the same values. On the mac make sure that
touch2sudo\_path is correct.

5) Start the authentication service on the mac:

````
$ launchctl load -w ~/Library/LaunchAgents/com.m4rkw.touchid-remote.plist
````

The -w flag indicates that this should run persistently, ie it will be loaded
when the system restarts.

6) Test that the authentication server works:

````
$ curl http://localhost:61111 -H "Auth: key1"
````

Replace "key1" with configured secret for key1. When you execute this command
you should see a touch2sudo prompt, if this is successfully validated the output
of the curl command will be the value of key2.

7) Add this line to the top of /etc/pam.d/sudo (but after the #%PAM-1.0
bangline):

````
auth sufficient /usr/lib64/security/pam_tid.so
````

8) Allow the USE\_TOUCHID environment variable to be passed from your client
machine to the server by adding it to the AcceptEnv list in
/etc/ssh/sshd\_config:

````
AcceptEnv LANG LC_* USE_TOUCHID
````

Note: restart sshd for this to take effect.

9) Configure the remote tunnel for your ssh connections on the mac, for example
a connection entry in ~/.ssh/config might look like:

````
Host server1
  Hostname server1.mydomain.com
  SendEnv USE_TOUCHID
  RemoteForward localhost:61111 localhost:61111
````

This ensures that whenever you connect to this server with ssh it will attempt
to initiate a tunnel back to the authentication server on the mac client.

Now ssh to the server using this new connection profile, eg:

````
$ ssh server1
````

and then type "sudo bash". You should see a touch2sudo prompt and when this is
successfully authenticated the sudo command will be accepted.

Of course only one such tunnel can exist because only one socket can listen on
localhost:61111 on the remote server, so if you open a second connection you
will see:

````
Warning: remote port forwarding failed for listen port 61111
````

However this doesn't matter and the second connection will still be able to
authenticate with sudo using the tunnel created with the first connection.

## Disclaimer

Use at your own risk, I am not responsible for any security issues that may
arise from using this software.

This project is not affiliated with Apple or the author of touch2sudo in any
way. "TouchID" and "iPhone" are trademarks of Apple.
