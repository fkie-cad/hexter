# Change Log


## v1.11.4 - 2026/08/12

- -ci case insensitive search now also works for -fu (utf-16le) in the ascii range



## v1.11.3 - 2026/08/11

- fixed /ci bug for /fu, which wrongly partly was applied to /fu searches too
- added change log
- refactoring



## v1.11.2 - 2026/08/07

- -e end parameter to limit file window to an hard end less than file size for printing and searching
- -mfc max find count parameter to limit find mode to a max number of found items
- forced breaking mode for stdin/stdout redirected calls
- some further ongoing refactoring



## v1.10.14 - 2026/07/31

- dropped unicode printing for complexity and security reasons



## v1.10.2 - 2026/07/28

- linux process int size fix



## v1.10.1 - 2026/07/24 

- column layout options
- hex value/group size options
- explicit offset printing
- search in break mode complies to length settings
- custom col size for some layouts
