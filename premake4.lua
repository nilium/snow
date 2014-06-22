--[[----------------------------------------------------------------------------
  premake4.lua -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
------------------------------------------------------------------------------]]


-- Set up snow table
snow = {}

--[[----------------------------------------------------------------------------
  join_arrays

    Joins zero or more arrays into a single array. Discards original keys in
    input tables.
------------------------------------------------------------------------------]]
function snow.join_arrays(...)
  local result = {}
  local inputs = {...}
  for ik,iv in pairs(inputs) do
    for jk,jv in pairs(iv) do
      result[#result + 1] = jv
    end
  end
  return result
end


function snow.tostring(e)
  if e == nil then
    return ""
  elseif type(e) == "function" then
    return snow.tostring(e())
  elseif type(e) == "number" then
    return tostring(e)
  elseif type(e) == "boolean" then
    if e then
      return "1"
    else
      return "0"
    end
  else
    return tostring(e)
  end
end


--[[----------------------------------------------------------------------------
  formatrb_string

    Returns arguments combined for format.rb
------------------------------------------------------------------------------]]
function snow.formatrb_string(t)
  local result = ""
  for k,v in pairs(t) do
    result = result .. " '" .. snow.tostring(k) .. "=" .. snow.tostring(v) .. "'"
  end
  return result
end


function snow.install(source_path, install_path, verbose)
  if verbose then
    print("Installing '" .. source_path .."' to '" .. install_path .. "'")
  end
  local done, errmsg = os.copyfile(source_path, install_path)
  if not done then
    print("os.copyfile: " .. errms)
    print("Unable to copy " .. source_path, " unable to continue installation")
    return false
  end
  return true
end


function snow.correct_dirsep(str)
  if os.is("windows") then
    return string.gsub(str, "/", "\\")
  else
    return string.gsub(str, "\\", "/")
  end
end


function snow.dirname(str)
  local last = string.findlast(str, "/", true)
  if last ~= nil then
    return string.sub(str, 0, last)
  end
  return str
end


function snow.mkdir_p(path)
  local dirsep = "/"
  if os.is("windows") then
    dirsep = "\\"
  end

  local components = string.explode(path, "/")
  local last_dir = 0

  do -- Make sure nothing along the way is a file
    local dir_build = ""
    for i, v in ipairs(components) do
      if v ~= "" then
        dir_build = dir_build .. dirsep .. v
        if os.isdir(dir_build) == true then
          last_dir = i
        elseif os.isfile(dir_build) then
          return false -- Cannot create dir if file of the same name exists already
        end
      end
    end -- for in components
  end -- do check path

  do
    local dir_build = ""
    for i, v in ipairs(components) do
      if #v > 0 then
        dir_build = dir_build .. dirsep .. v
        if i > last_dir then
          local done, errmsg = os.mkdir(dir_build)
          if not done then
            print("Unable to create directory: " .. dir_build)
            return false
          end -- if not done
        end -- if i > last_dir
      end
    end -- for in components
  end -- do build directory

  return true
end -- mkdir_p



-- Main build configuration

solution "snow"
configurations { "debug", "release" }

-- Build options
newoption {
  trigger = "with-exceptions",
  description = "Enables exceptions -- if disabled, throws are replaced with std::abort()"
}

newoption {
  trigger = "with-server",
  description = "Enables the server code in the engine build."
}

if os.is("macosx") then
  newoption {
    trigger = "gcc",
    description = "Specifies use of GCC on OS X"
  }
end


--[[ snow_c_libs project ------------------------------------------]] do
project       "snow_c_libs"
kind          "StaticLib"
targetdir     "lib"

language      "C"
files         { "src/ext/sqlite3.c", "src/ext/stb_image.c" }
buildoptions  { "-arch x86_64" }

end


--[[ snow_objcxx_libs project ------------------------------------------]] do
project       "snow_objcxx_libs"
kind          "StaticLib"
targetdir     "lib"

language      "C++"
files( os.matchfiles("src/**.mm") )

flags         { "FloatStrict", "NoRTTI" }

buildoptions  { "-arch x86_64" }
buildoptions  { "-include src/snow.objc.hh" }

buildoptions  { "-std=c++11" }

configuration { "macosx" }
buildoptions  { "-stdlib=libc++" }
links         { "c++" }

configuration { "not with-exceptions" }
flags         { "NoExceptions" }

configuration { "macosx", "release" }
buildoptions  { "-O3" }

configuration "release"
defines       { "NDEBUG" }

configuration "debug"
defines       { "DEBUG" }
flags         { "Symbols" }

end


--[[ snowhash project ---------------------------------------------]] do
project       "snowhash"
language      "C++"
kind          "ConsoleApp"
files         { "snowhash.cc" }

-- Link snow-common
linkoptions   { '`pkg-config --libs snow-common`' }
buildoptions  { '`pkg-config --cflags snow-common`' }

buildoptions  { "-std=c++11" }

configuration { "macosx" }
buildoptions  { "-stdlib=libc++" }
links         { "c++" }

configuration { "not with-exceptions" }
flags         { "NoExceptions" }

configuration { "macosx", "release" }
buildoptions  { "-O3" }

configuration "release"
defines       { "NDEBUG" }

configuration "debug"
defines       { "DEBUG" }
flags         { "Symbols" }

end


--[[ snowhost project ---------------------------------------------]] do
project       "snowhost"
language      "C++"
kind          "ConsoleApp"
files         { "host.cc" }

-- Link snow-common
linkoptions   { '`pkg-config --libs snow-common`' }
buildoptions  { '`pkg-config --cflags snow-common`' }

links { "zmq" }

buildoptions  { "-std=c++11" }

configuration { "macosx" }
buildoptions  { "-stdlib=libc++" }
links         { "c++" }

configuration { "not with-exceptions" }
flags         { "NoExceptions" }

configuration { "macosx", "release" }
buildoptions  { "-O3" }

configuration "release"
defines       { "NDEBUG" }

configuration "debug"
defines       { "DEBUG" }
flags         { "Symbols" }

end


--[[ snow project -------------------------------------------------]] do
project       "snow"
targetdir     "bin"
kind          "WindowedApp"


-- Option defaults
local g_exclude_suffixes = {}


buildoptions { "-include '" .. path.getabsolute('src') .. "/snow.cxx.hh'"}


-- Compiler flags
local g_version = "0.0.1"
language      "C++"
flags         { "FloatStrict", "NoRTTI" }
objdir        "obj"


-- Libraries
links         { "zmq" }
links         { "physfs", "snow_c_libs", "snow_objcxx_libs" }
linkoptions   { '`fltk-config --ldflags`' }
buildoptions  { '`fltk-config --cxxflags`' }


-- Add sources/include directories
includedirs   { "include" }
local cxx_files = snow.join_arrays(os.matchfiles("src/**.cc"), os.matchfiles("src/**.cxx"), os.matchfiles("src/**.cpp"))
files(cxx_files)
excludes "src/snow.cc" -- pch only

configuration "not with-server"
excludes      { "src/server/*" }

configuration { "macosx", "release" }
buildoptions  { "-O3" }

configuration "release"
defines       { "NDEBUG" }

configuration "debug"
defines       { "DEBUG" }
flags         { "Symbols" }


local g_build_config_opts = {
  USE_EXCEPTIONS = false,
  USE_SERVER = false
}

if _OPTIONS["with-exceptions"] ~= nil then
  g_build_config_opts.USE_EXCEPTIONS = not not _OPTIONS["with-exceptions"]
end

if _OPTIONS["with-server"] ~= nil then
  g_build_config_opts.USE_SERVER = not not _OPTIONS["with-server"]
end

-- Exceptions
configuration "not with-exceptions"
flags         { "NoExceptions" }

-- OS X specific options
configuration "macosx"
links         { "Cocoa.framework" }
links         { "OpenGL.framework" }
links         { "IOKit.framework" }

buildoptions  { "-arch x86_64" }
linkoptions   { "-ObjC++", "-headerpad_max_install_names", "-arch x86_64" }

configuration {}
buildoptions  { "-std=c++11" }

configuration { "macosx" }
buildoptions  { "-stdlib=libc++" }
links         { "c++" }

local target_path_osx = "bin/snow.app/Contents/MacOS/snow"
local change_libs_osx = {
  ["libphysfs.1.dylib"]                                = "@executable_path/../Frameworks/libphysfs.dylib",
  ["/usr/local/lib/libzmq.3.dylib"]                    = "@executable_path/../Frameworks/libzmq.dylib",
  ["/usr/local/lib/libenet.2.dylib"]                   = "@executable_path/../Frameworks/libenet.dylib"
}

do
  local changes_string = ''

  for from,to in pairs(change_libs_osx) do
    changes_string = changes_string .. " -change '" .. from .. "' '" .. to .. "'"
  end

  if #changes_string > 0 then
    changes_string = changes_string .. " '" .. target_path_osx .. "'"
    postbuildcommands { "install_name_tool" .. changes_string }
  end
end

-- files { "src/main.mm" }
-- excludes { "src/main.cc" }

-- pkg-config packages
configuration {}
g_dynamic_libs = { }
g_static_libs = { "snow-common", "libenet", "glfw3" }

if #g_static_libs > 0 then
  linkoptions { "`pkg-config --libs " .. table.concat(g_static_libs, " ") .. "`"}
end

if #g_dynamic_libs > 0 then
  linkoptions { "`pkg-config --libs " .. table.concat(g_dynamic_libs, " ") .. "`"}
end

if (#g_dynamic_libs + #g_static_libs) > 0 then
  local combined_libs = table.concat(snow.join_arrays(g_dynamic_libs, g_static_libs), " ")
  buildoptions { "`pkg-config --cflags " .. combined_libs .. "`"}
end

-- Generate build-config/pkg-config
if _ACTION and _ACTION ~= "install" then
  -- Generate build-config.hh
  local config_src = "'src/build-config.hh.in'"
  local config_dst = "'src/build-config.hh'"

  print("Generating 'src/build-config.hh'...")
  os.execute("./format.rb " .. config_src .. " " .. config_dst .. snow.formatrb_string(g_build_config_opts))
end

end
