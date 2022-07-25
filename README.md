# SecureClock

**For linux only.**

Update and display your time in a secure way.

## Build

```
gcc -o secureclock secureclock.c -z noexecstack -fstack-protector-all -D_GNU_SOURCE -lcap -lpthread -Wall -Wextra -Wformat -Wformat-security -Wduplicated-cond -Wfloat-equal -Wshadow -Wconversion -Wjump-misses-init -Wlogical-not-parentheses -Wnull-dereference -D_FORTIFY_SOURCE=2 -pie -fPIE -Wl,-z,relro -Wl,-z,now -O4

gcc -o settime settime.c -z noexecstack -fstack-protector-all -D_GNU_SOURCE -lcap -Wall -Wextra -Wformat -Wformat-security -Wduplicated-cond -Wfloat-equal -Wshadow -Wconversion -Wjump-misses-init -Wlogical-not-parentheses -Wnull-dereference -D_FORTIFY_SOURCE=2 -pie -fPIE -Wl,-z,relro -Wl,-z,now -O4
```

## Installation

Set the appropriate capabilities. `sudo setcap cap_sys_time+ep settime`

Create the configuration file in `/etc/secureclock.conf` with the server port.

```
1514
```

## Overview

Get the current time :
```
Instance c[li]/s[erver]/e[xit]: c
Method s[et]/g[et]: g
Format string:
> y/M/D H:I:S
2022/07/25 15:18:16
```

Update your local time:
```
Instance c[li]/s[erver]/e[xit]: c
Method s[et]/g[et]: s
Date string `y-m-d h:i:s`: 2022-06-14 08:00:00
Date successfully updated
```

Start your server and get the time remotely:
```
Instance c[li]/s[erver]/e[xit]: s
Server running on port 1514...

# In another shell
./client y/M/D
2022/06/14
2022/06/14
2022/06/14
```
