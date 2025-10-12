# Resource Manager Integration Tests <!-- omit in toc -->

This folder contains test programs (not unit test code) that can be used to
explore and test the components on the QNX target. They don't have pass/fail
semantics, but instead provide simple skeletons that can be used to check
functionality works as expected through manual inspection.

The following sections describe the test cases that can be done on the target
with the built binaries.

## 1. Minimal Resource Manager - Single Threaded

The tool `dev-resmgr` provides a simple resource manager `/dev/sample` that can
be used to see the dispatch loop in progress.

## 2. Test Cases

### 2.1. Exit from a Signal Handler

Send either a `SIGINT` (e.g. Ctrl-C) or `SIGTERM` and the resource manager will
shutdown cleanly.

### 2.2. Stopped

If you stop the resource manager (e.g. `SIGSTOP` or Ctrl-Z), it will no longer
be able to reply to any messages. If you:

```
ls -la /dev
```

then listing the directory will block forever, until the process is running
again. This is because clients are SEND-BLOCKED or REPLY-BLOCKED (depending on
when the signal arrived, if before or after receiving the message from `ls` to
do a `stat`).
