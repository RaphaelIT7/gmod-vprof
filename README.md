This Project tries to improve vprof by adding new commands and convars.  
NOTE: This project currently only works on Linux 32x  

## What does this add?
Currently this module adds two new convars.

### vprof_showhooks
Causes VProf to show the name of the Lua hooks getting called.

### vprof_exportreport
Causes VProf to export a report into a file in the `vprof/` folder.

## How to load
You can load it like any normal module by using require, but you can also load it like a server plugin.  

```lua
require("vprof")
```

```lua
Plugin
{
	file		"lua/bin/gmsv_vprof_linux.dll"
}
```