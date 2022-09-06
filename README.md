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

*Optionnal*  
To get the time remotely, the configuration file in `/etc/secureclock.conf` must be created with the server port.

```
1514
```

## Examples

```

    ┌─────────────────────┐
    │ Secure Clock v1.1.0 │
    └─────────────────────┘

§ help
 > help : Display the list of commands and arguments description.
 > exit : Exit the program.
 > get  : Get the local time in a specific format.
 > set  : Update the local time.
 > up   : Start the local server.
 > kill : Stop the local server.

§ set 2022-02-03 12:23:25
    Success update date

§ get D/M/y H:I, ok ?
    06/09/2022 19:06, ok ?

§ help help
 > help [ COMMAND ]
 # Display help for available commands.
 # COMMAND: optionnal parameter
```
