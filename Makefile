.PHONY: all build

all: build

build:
	if [ "`uname`" != "Darwin" ] ; then gcc -fPIC -c pam_tid.c ; fi
	if [ "`uname`" != "Darwin" ] ; then ld -x --shared -lconfig -lpam -o pam_tid.so pam_tid.o ; fi

install: build
	if [ "`uname`" != "Darwin" ] ; then cp pam_tid.so /usr/lib64/security/ ; fi
	if [ "`uname`" != "Darwin" -a ! -e /etc/touchid-remote.conf ] ; then cp touchid-remote.conf.example-linux /etc/touchid-remote.conf ; chown root:root /etc/touchid-remote.conf ; chmod 600 /etc/touchid-remote.conf ; fi
	if [ "`uname`" != "Darwin" -a ! -e ~/.touchid-remote ] ; then mkdir ~/.touchid-remote ; fi
	if [ "`uname`" != "Darwin" -a ! -e ~/.touchid-remote/touchid-remote.conf ] ; then cp touchid-remote.conf.example-mac ~/.touchid-remote/touchid-remote.conf ; chmod 600 ~/.touchid-remote/touchid-remote.conf ; fi
	if [ "`uname`" != "Darwin" ] ; then cp server.py ~/.touchid-remote/ ; fi
	if [ "`uname`" != "Darwin" -a ! -e ~/Library/LaunchAgents ] ; then mkdir -p ~/Library/LaunchAgents ; fi
	if [ "`uname`" != "Darwin" ] ; then cp com.m4rkw.touchid-remote.plist ~/Library/LaunchAgents/ ; fi
