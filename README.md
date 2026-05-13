This project tries to improve vprof by adding new commands and convars.  
NOTE: This project currently only works on 32x  

## What does this add?
Currently, this module adds two new convars.

### vprof_showhooks
#### Enabled by default
Causes VProf to show the name of the Lua hooks getting called.  
> Gmod request: https://github.com/Facepunch/garrysmod-requests/issues/2374

`CLuaInterface::CallFunctionProtected` entries show the source file and line number of the called function (e.g. `CLuaInterface::CallFunctionProtected(gamemode/init.lua:42)`). In the exported report, short IDs like `LUA#0001` are replaced with the full label.

#### Example on Linux
`CLuaGamemode::Call` is listed twice because the first one is our custom one and the second one is the original.
```lua
       |  |  |  |  |  |  CLuaGamemode::Call (Think)
       |  |  |  |  |  |  |  CLuaGamemode::Call
       |  |  |  |  |  |  |  |  CBaseLuaInterface::GetType
       |  |  |  |  |  |  |  |  CLuaInterface::CallFunctionProtected(gamemode/init.lua:42)
```

#### Example on Windows
On Windows, the results currently are different because I can't detour the CLuaGamemode without breaking it :<  
So now you see the hook name in `CLuaInterface::PushPooledString (Hook name here)`
```lua
|  |  CLuaGamemode::Call
|  |  |  CLuaInterface::PushPooledString (Tick)
|  |  |  CBaseLuaInterface::GetType
|  |  |  CLuaInterface::CallFunctionProtected(42@gamemode/init.lua)
|  |  |  CLuaInterface::PushPooledString (Think)
```

### vprof_exportreport
#### Enabled by default
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