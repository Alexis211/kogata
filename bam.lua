--
-- Redefine output function to put everything in build/
--

function BuildOutput(mode)
	local basepath = mode and ("build/" .. mode) or "build"

	return function (settings, fname)
		if fname:sub(1, 4) == "src/" or fname:sub(1, 4) == "res/" then
			fname = fname:sub(5)
		end
		local out = PathJoin(basepath, PathBase(fname) .. settings.config_ext)
		return out
	end
end

--
-- Define custom settings
--

local host_settings = NewSettings()
local common_settings = NewSettings()

if os.getenv('CC') and string.match(os.getenv('CC'), '.*analyzer$') then
	print("Detected clang-analyzer")
	SetDriversGCC(host_settings)
	host_settings.cc.exe_c = 'CCC_CC=gcc ' .. os.getenv('CC')
	host_settings.cc.exe_cxx = 'CCC_CXX=g++ ' .. os.getenv('CXX')

	SetDriversGCC(common_settings)
	common_settings.cc.flags:Add('-U__linux__')
	common_settings.cc.exe_c = 'CCC_CC=i586-elf-gcc ' .. os.getenv('CC')
	common_settings.cc.exe_cxx = 'CCC_CXX=i586-elf-g++ ' .. os.getenv('CXX')
	common_settings.link.exe = 'CCC_CC=i586-elf-gcc ' .. os.getenv('CC')
else
	common_settings.cc.exe_c = "i586-elf-gcc"
	common_settings.cc.exe_cxx = "i586-elf-g++"
	common_settings.link.exe = "i586-elf-gcc"
end


host_settings.cc.Output = BuildOutput(nil)
host_settings.cc.extension = ".host.o"
host_settings.cc.includes:Add("src/lib/include/proto",
							  "src/common/include")
host_settings.link.extension = ".bin"


common_settings.compile.mappings['s'] = function(settings, input)
	local output = settings.cc.Output(settings, input) .. settings.cc.extension
	AddJob(output, "nasm " .. input,
		   "nasm -felf -g -o " .. output .. " " .. input)
	AddDependency(output, input)
	return output
end

common_settings.cc.Output = BuildOutput(nil)
common_settings.cc.includes:Add("src/common/include", ".")
common_settings.cc.flags:Add("-m32",
							 "-ffreestanding",
							 "-std=gnu99",
							 "-Wall", "-Wextra", "-Werror",
							 "-Wno-unused-parameter",
							 "-Wno-unused-function")

common_settings.link.extension = ".bin"
common_settings.link.flags:Add("-ffreestanding",
							   "-nostdlib")
common_settings.link.libs:Add("gcc")
common_settings.link.Output = BuildOutput(nil)


local user_settings = TableDeepCopy(common_settings)
user_settings.cc.includes:Add('src/lib/include')

local base_settings = {
	host_settings = host_settings,
	common_settings = common_settings,
	user_settings = user_settings
}

--
-- Require build scripts for all components
--

local fonts = require('res/fonts/bam')(base_settings)
local keymaps = require('res/keymaps/bam')(base_settings)

local function cdrom(name, settings)
	for _, s in pairs(settings) do
		s.cc.Output = BuildOutput(name)
		s.link.Output = BuildOutput(name)
	end

	local common = require('src/common/bam')(settings)
	local kernel = require('src/kernel/bam')(settings, common)
	local lib = require('src/lib/bam')(settings, common)
	local sysbin = require('src/sysbin/bam')(settings, lib)

	if name == "dev" then
		dev_kernel = kernel
	end

	local cdrom = "cdrom." .. name .. ".iso"
	AddJob(cdrom, "building ISO", "./make_cdrom.sh " .. name)
	AddDependency(cdrom, kernel.bin, sysbin, fonts, keymaps)
	AddDependency(cdrom, CollectRecursive('src/syslua/*.lua'))
	AddDependency(cdrom, CollectRecursive('src/sysapp/*.lua'))

	--
	-- Script for running tests
	--

	local tests = {
		require('src/tests/slab_test/bam')(settings),
		require('src/tests/ktests/bam')(settings, common, kernel.obj),
		require('src/tests/utests/bam')(settings, kernel.bin, lib)
	}

	PseudoTarget("test." .. name, tests)

	return cdrom
end


local dev_settings = TableDeepCopy(base_settings)
for _, s in pairs(dev_settings) do
	s.cc.flags:Add("-g", "-O0")
end
local dev_cdrom = cdrom("dev", dev_settings)

local rel_settings = TableDeepCopy(base_settings)
for _, s in pairs(rel_settings) do
	s.cc.flags:Add("-Os", "-flto")
	s.link.flags:Add("-O3", "-Os", "-flto", "-Wl,--gc-sections")
	
	-- Maybe do this, goes with --gc-sections
	-- s.cc.flags:Add("-ffunction-sections", "-fdata-sections")
end
local rel_cdrom = cdrom("rel", rel_settings)

DefaultTarget(dev_cdrom)

