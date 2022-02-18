#!/usr/bin/env python3
#
# TouchID Remote server
# https://github.com/m4rkw/touchid-remote

import os
import sys
import signal
import socket
import time
import pwd
import re

CONFIG_FILE = pwd.getpwuid(os.getuid()).pw_dir + '/.touchid-remote/touchid-remote.conf'

class TouchIDRemoteServer:

  def __init__(self):
    self.load_config()


  def load_config(self):
    if not os.path.exists(CONFIG_FILE):
      raise Exception("config file not found: %s" % (CONFIG_FILE))

    self.config = {}

    with open(CONFIG_FILE, 'r') as f:
      for line in f:
        line = line.rstrip()

        match = re.match('^(.*?): (.*)$', line)

        if match:
          if match.group(2).isdigit():
            self.config[match.group(1)] = int(match.group(2))
          else:
            if match.group(2)[0] == '"' and match.group(2)[-1] == '"':
              self.config[match.group(1)] = match.group(2)[1:len(match.group(2))-1]
            else:
              self.config[match.group(1)] = match.group(2)


  def main(self):
    signal.signal(signal.SIGCHLD, signal.SIG_IGN)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", self.config['server_port']))
    s.listen(100)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    while 1:
      (cs, addr) = s.accept()
      pid = os.fork()

      if pid == 0:
        s.close()
        self.handle(cs, addr)
        sys.exit(0)

      cs.close()


  def handle(self, cs, addr):
    req = cs.recv(1024).decode()

    resp =  "HTTP/1.1 200 OK\n"
    resp += "Server:localhost\n"
    resp += "Content-type:text/plain\n\n"

    msg = '1'

    if 'Auth:' in req:
      key = req.split('Auth: ')[1].split("\r")[0]

      if key == self.config['key1']:
        rc = os.system(self.config['touch2sudo_path'])

        if rc == 0:
          msg = self.config['key2']

    resp += msg 

    cs.sendall(resp.encode('utf-8'))
    cs.close()


t = TouchIDRemoteServer()
t.main()
