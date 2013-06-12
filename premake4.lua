--[[----------------------------------------------------------------------------
  premake4.lua -- Noel Cower
  This file is a part of the snow engine.
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


--[[----------------------------------------------------------------------------
  get_headers

    Retrieves a list of all headers, excluding any that match the suffixes in
    the excluded_suffixes argument (if provided).
------------------------------------------------------------------------------]]
function snow.get_headers(excluded_suffixes)
  local headers = snow.join_arrays(
    os.matchfiles("include/**.hh"),
    os.matchfiles("include/**.cc"),
    os.matchfiles("include/*.hh"),
    os.matchfiles("include/*.cc") )
  local swap = {}
  for header_k, header_path in pairs(headers) do
    if excluded_suffixes then
      -- Check for exclusions
      for excl_k, excl_suffix in pairs(excluded_suffixes) do
        if string.endswith(header_path, excl_suffix) then
          print(excl_k .. ": Excluding " .. header_path)
          header_path = nil
          break
        end -- if matches excluded header
      end -- for in excluded_suffixes
    end -- if excluded_suffixes

    if header_path then
      swap[header_path] = header_path
    end
  end -- for in headers
  return swap
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

project "snow_c_libs"
kind "StaticLib"
targetdir "lib"
language "C"
files { "src/ext/sqlite3.c", "src/ext/stb_image.c" }
buildoptions { "-arch x86_64" }

project "snow"

configuration {}

targetdir "bin"
kind "WindowedApp"

links { "angelscript" }

-- Option defaults
g_exclude_suffixes = {}

-- Build options
newoption {
  trigger = "no-exceptions",
  description = "Disables exceptions in snow-common -- replaces throws with exit(1)"
}

newoption {
  trigger = "no-server",
  description = "Disables the server code in the engine build"
}

newoption {
  trigger = "with-tbb",
  description = "Specify a path to Intel's Threading Building Blocks",
  value = "/path/to/tbb"
}

if os.is("macosx") then
  newoption {
    trigger = "gcc",
    description = "Specifies use of GCC on OS X"
  }
end

-- Compiler flags
g_version = "0.0.1"
language "C++"
flags { "FloatStrict", "NoRTTI" }
objdir "obj"

-- Libraries
links { "zmq" }
links { "physfs", "snow_c_libs" }
linkoptions { '`fltk-config --ldflags`' }
buildoptions { '`fltk-config --cxxflags`' }

-- Add sources/include directories
includedirs { "include" }
local cxx_files = snow.join_arrays(os.matchfiles("src/**.cc"), os.matchfiles("src/**.cxx"), os.matchfiles("src/**.cpp"))
files(cxx_files)

configuration "no-server"
excludes { "src/server/*.cc" }

configuration "release"
defines { "NDEBUG" }

configuration { "macosx", "release" }
buildoptions { "-O3" }

configuration "debug"
defines { "DEBUG" }
flags { "Symbols" }

g_build_config_opts = {
  USE_EXCEPTIONS = true,
  USE_SERVER = true
}

if _OPTIONS["no-exceptions"] ~= nil then
  g_build_config_opts.USE_EXCEPTIONS = not _OPTIONS["no-exceptions"]
end

if _OPTIONS["no-server"] ~= nil then
  g_build_config_opts.USE_SERVER = not _OPTIONS["no-server"]
end

-- Exceptions
configuration "no-exceptions"
flags { "NoExceptions" }

-- OS X specific options
configuration "macosx"
links { "Cocoa.framework" }
links { "OpenGL.framework" }
links { "IOKit.framework" }

buildoptions { "-arch x86_64" }
linkoptions { "-ObjC++", "-headerpad_max_install_names", "-arch x86_64" }

configuration {}
buildoptions { "-std=c++11" }

configuration { "macosx" }
buildoptions { "-stdlib=libc++" }
links { "c++" }

local target_path_osx = "bin/snow.app/Contents/MacOS/snow"
local change_libs_osx = {
  ["libphysfs.1.dylib"]                                = "@executable_path/../Frameworks/libphysfs.dylib",
  ["libtbb_debug.dylib"]                               = "@executable_path/../Frameworks/libtbb_debug.dylib",
  ["libtbb.dylib"]                                     = "@executable_path/../Frameworks/libtbb.dylib",
  ["libtbbmalloc.dylib"]                               = "@executable_path/../Frameworks/libtbbmalloc.dylib",
  ["libtbbmalloc_debug.dylib"]                         = "@executable_path/../Frameworks/libtbbmalloc_debug.dylib",
  ["/usr/local/lib/libenet.2.dylib"]                   = "@executable_path/../Frameworks/libenet.dylib"
  --["/usr/local/opt/sqlite/lib/libsqlite3.0.8.6.dylib"] = "@executable_path/../Frameworks/libsqlite3.dylib",
  -- ["/usr/local/opt/openssl/lib/libssl.1.0.0.dylib"]    = "@executable_path/../Frameworks/libcrypto.dylib",
  -- ["/usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib"] = "@executable_path/../Frameworks/libssl.dylib"
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

-- TBB
local build_with_tbb = false
if build_with_tbb then
  configuration {}
  local tbb_path = "~/source/tbb-4-1"
  if _OPTIONS["with-tbb"] then
    tbb_path = _OPTIONS["with-tbb"]
  end

  libdirs { tbb_path .. "/lib" }
  includedirs { tbb_path .. "/include" }

  configuration "debug"
  links { "tbb_debug" }
  -- links { "tbbmalloc_debug" }

  configuration "not debug"
  links { "tbb" }
  -- links { "tbbmalloc" }
end

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
