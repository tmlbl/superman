superman
========

A supervisor alternative in pure C. Superman manages a set of processes from a
YAML config file. It will restart any of the processes if they exit, and it can
redirect STDOUT and STDERR to a file for logging purposes. That is all it does
for now.

Example YAML:
```yaml
sleeper1:
  command: time sleep 1
  file: /var/log/sleepr
```
