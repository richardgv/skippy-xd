#!/bin/sh

killSkippyXd() {

  killall 'skippy-xd'

  if [ -f '/tmp/skippy-xd-fifo' ]
  then
    rm /tmp/skippy-xd-fifo
    touch /tmp/skippy-xd-fifo
  fi

  exit $?

}

toggleSkippyXd() {

  skippyDaemonStartCommand="skippy-xd --start-daemon"
  psSkippyOut="`pgrep -f 'skippy-xd '`"
  psSkippyActivateOut="`pgrep -f 'skippy-xd --activate-window-picker'`"
  psSkippyToggleOut="`pgrep -f 'skippy-xd --toggle-window-picker'`"

  skippyConfig="$1"

  if [ -f "$skippyConfig" ]
  then
    skippyDaemonStartCommand="$skippyDaemonStartCommand --config $skippyConfig"
  fi

  ## IF, when calling skippy-xd, to start the window picker, 
  ## a process to do so already exists, then skippy-xd is stuck, 
  ## so we must clear its queue and restart its daemon.
  ## ELSE, if the skippy-xd daemon has not been started,
  ## we should start it.  

  if [ ! -z "$psSkippyActivateOut" ] || [ ! -z "$psSkippyToggleOut" ]
  then
    killSkippyXd
    $skippyDaemonStartCommand &
  elif [ -z "$psSkippyOut" ]
  then
    $skippyDaemonStartCommand &
  fi

  skippy-xd --toggle-window-picker

  exit $?

}

(toggleSkippyXd "$@")
