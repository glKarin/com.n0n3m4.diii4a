local m = {}
local toolsets = {gcc = {}, clang = {}, mingw = {}, msc = {}}
local toolset = nil
local cxx = nil

function toolsets.gcc.is_cxx_compileable(code, include_dirs)
  local include_dirs = include_dirs or {}
  local code_file_path = _MAIN_SCRIPT_DIR .. "/tmp.cpp"
  local code_file = io.open(code_file_path, "w")

  code_file:write(code)
  code_file:close()

  local include_dirs_str = ""

  for _, include_dir in ipairs(include_dirs) do
    include_dirs_str = include_dirs_str .. "-I" .. include_dir
  end

  local cmd = string.format("%s %s -c %s", cxx, include_dirs_str, code_file_path)
  local _, _, rc = os.execute(cmd)
  local result = false

  if rc == 0 then
    result = true
  end

  return result
end

function toolsets.msc.is_cxx_compileable(code, include_dirs)
  local include_dirs = include_dirs or {}
  local code_file_path = _XASHAIN_SCRIPT_DIR .. "/tmp.cpp"
  local code_file = io.open(code_file_path, "w")

  code_file:write(code)
  code_file:close()

  local include_dirs_str = ""

  for _, include_dir in ipairs(include_dirs) do
    include_dirs_str = include_dirs_str .. "/I" .. include_dir
  end

  local cmd = string.format("%s %s /c %s", cxx, include_dirs_str, code_file_path)
  local _, _, rc = os.execute(cmd)
  local result = false

  if rc == 0 then
    result = true
  end

  return result
end

toolsets.clang.is_cxx_compileable = toolsets.gcc.is_cxx_compileable
toolsets.mingw.is_cxx_compileable = toolsets.gcc.is_cxx_compileable

function m.is_cxx_symbol_defined(symbol, headers, include_dirs)
  local headers = headers or {}
  local include_dirs = include_dirs or {}
  local code = ""

  for _, header in ipairs(headers) do
    code = code .. string.format("#include <%s>\n", header)
  end

  code = code .. string.format("#ifndef %s\n.\n#endif", symbol)

  return m.is_cxx_compileable(code, include_dirs)
end

function m.is_cxx_header_exist(header, include_dirs)
  local include_dirs = include_dirs or {}
  local code = ""

  code = code .. string.format("#include <%s>\n", header)

  return m.is_cxx_compileable(code, include_dirs)
end

function m.get_arch()
  local arch = m.arch

  if not arch then
    if m.is_cxx_symbol_defined("XASH_AMD64", {"build.h"}, {"public"}) then
      arch = "amd64"
    elseif m.is_cxx_symbol_defined("XASH_X86", {"build.h"}, {"public"}) then
      arch = "x86"
    elseif m.is_cxx_symbol_defined("XASH_ARM", {"build.h"}, {"public"}) then
      if m.is_cxx_symbol_defined("XASH_64BIT", {"build.h"}, {"public"}) then
        arch = "arm64"
      else
        arch = "armv"

        if m.is_cxx_symbol_defined("XASH_ARMv8", {"build.h"}, {"public"}) then
          arch = arch .. "8_32"
        elseif m.is_cxx_symbol_defined("XASH_ARMv7", {"build.h"}, {"public"}) then
          arch = arch .. "7"
        elseif m.is_cxx_symbol_defined("XASH_ARMv6", {"build.h"}, {"public"}) then
          arch = arch .. "6"
        elseif m.is_cxx_symbol_defined("XASH_ARMv5", {"build.h"}, {"public"}) then
          arch = arch .. "5"
        elseif m.is_cxx_symbol_defined("XASH_ARMv4", {"build.h"}, {"public"}) then
          arch = arch .. "4"
        end

        if m.is_cxx_symbol_defined("XASH_ARM_HARDFP", {"build.h"}, {"public"}) then
          arch = arch .. "hf"
        else
          arch = arch .. "l"
        end
      end
    elseif m.is_cxx_symbol_defined("XASH_MIPS", {"build.h"}, {"public"}) then
      arch = "mips"

      if m.is_cxx_symbol_defined("XASH_64BIT", {"build.h"}, {"public"}) then
        arch = arch .. "64"
      end

      if m.is_cxx_symbol_defined("XASH_LITTLE_ENDIAN", {"build.h"}, {"public"}) then
        arch = arch .. "el"
      end
    elseif m.is_cxx_symbol_defined("XASH_RISCV", {"build.h"}, {"public"}) then
      arch = "riscv"

      if m.is_cxx_symbol_defined("XASH_64BIT", {"build.h"}, {"public"}) then
        arch = arch .. "64"
      else
        arch = arch .. "32"
      end

      if m.is_cxx_symbol_defined("XASH_RISCV_DOUBLEFP", {"build.h"}, {"public"}) then
        arch = arch .. "d"
      else
        arch = arch .. "f"
      end
    elseif m.is_cxx_symbol_defined("XASH_JS", {"build.h"}, {"public"}) then
      arch = "javascript"
    elseif m.is_cxx_symbol_defined("XASH_E2K", {"build.h"}, {"public"}) then
      arch = "e2k"
    end
  end

  return arch
end

function m.get_os()
  local dest_os = os.target()

  if dest_os == "bsd" then
    dest_os = m.bsd_flavour

    if not dest_os then
      if m.is_cxx_symbol_defined("XASH_FREEBSD", {"build.h"}, {"public"}) then
        dest_os = "freebsd"
      elseif m.is_cxx_symbol_defined("XASH_NETBSD", {"build.h"}, {"public"}) then
        dest_os = "netbsd"
      elseif m.is_cxx_symbol_defined("XASH_OPENBSD", {"build.h"}, {"public"}) then
        dest_os = "openbsd"
      end
    end
  end

  return dest_os
end

function m.get_lib_suffix()
  local dest_os = m.get_os()
  local arch = m.get_arch()

  if dest_os == "windows" or dest_os == "linux" or dest_os == "macosx" then
    dest_os = nil
  elseif dest_os == "android" then
    dest_os = nil
    arch = nil
  end

  if arch == "x86" then
    arch = nil
  end

  local suffix = ""

  if dest_os and arch then
    suffix = string.format("%s_%s", dest_os, arch)
  elseif arch then
    suffix = string.format("_%s", arch)
  end

  return suffix
end

function m.find_cxx_compiler()
  local prefix = m.prefix or ""
  local host_os = os.host()

  toolset = _OPTIONS["cc"]

  if not toolset then
    if host_os == "windows" then
      toolset = "msc"
    elseif host_os == "macosx" then
      toolset = "clang"
    else
      toolset = "gcc"
    end
  end

  if toolset == "gcc" or "mingw" then
    cxx = prefix .. "g++"
  elseif toolset == "clang" then
    cxx = prefix .. "clang++"
  elseif toolset == "msc" then
    cxx = "cl.exe"
  end

  local is_found = false

  if cxx then
    local cmd = ""

    if toolset == "msc" then
      cmd = cxx .. " /HELP"
    else
      cmd = cxx .. " --help"
    end

    _, _, rc = os.execute(cmd)

    if rc == 0 then
      is_found = true
      m.is_cxx_compileable = toolsets[toolset].is_cxx_compileable
    end
  end

  return is_found
end

return m
