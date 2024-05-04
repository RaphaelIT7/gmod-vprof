This project tries to improve vprof by adding new commands and convars.  
NOTE: This project currently only works on 32x  

## What does this add?
Currently, this module adds two new convars.

### vprof_showhooks
#### Enabled by default
Causes VProf to show the name of the Lua hooks getting called.  
> Gmod request: https://github.com/Facepunch/garrysmod-requests/issues/2374

#### Example on Linux
`CLuaGamemode::Call` is listed twice because the first one is our custom one and the second one is the original.
```lua
       |  |  |  |  |  |  CLuaGamemode::Call (Think)
       |  |  |  |  |  |  |  CLuaGamemode::Call
       |  |  |  |  |  |  |  |  CBaseLuaInterface::GetType
       |  |  |  |  |  |  |  |  CLuaInterface::CallFunctionProtected
```

#### Example on Windows
On Windows, the results currently are different because I can't detour the CLuaGamemode without breaking it :<  
So now you see the hook name in `CLuaInterface::PushPooledString (Hook name here)`
```lua
|  |  CLuaGamemode::Call
|  |  |  CLuaInterface::PushPooledString (Tick)
|  |  |  CBaseLuaInterface::GetType
|  |  |  CLuaInterface::CallFunctionProtected
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