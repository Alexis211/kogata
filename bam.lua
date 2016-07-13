--
-- Redefine output function to put everything in build/
--

function BuildOutput(settings, fname)
	if fname:sub(1, 4) == "src/" then
		fname = fname:sub(5)
	end
	local out = PathJoin("build", PathBase(fname) .. settings.config_ext)
	return out
end

--
-- Define custom settings
--

host_settings = NewSettings()
host_settings.cc.Output = BuildOutput
host_settings.cc.extension = ".host.o"
host_settings.cc.includes:Add("src/lib/include/proto",
							  "src/common/include")
host_settings.link.extension = ".bin"

common_settings = NewSettings()

common_settings.compile.mappings['s'] = function(settings, input)
	local output = BuildOutput(settings, input) .. settings.cc.extension
	AddJob(output, "nasm " .. input,
		   "nasm -felf -g -o " .. output .. " " .. input)
	AddDependency(output, input)
	return output
end

common_settings.cc.exe_c = "i586-elf-gcc"
common_settings.cc.exe_cxx = "i586-elf-g++"
common_settings.cc.Output = BuildOutput
common_settings.cc.includes:Add("src/common/include", ".")
common_settings.cc.flags:Add("-m32",
							 "-ffreestanding",
							 "-std=gnu99",
							 "-Wall", "-Wextra", "-Werror",
							 "-Wno-unused-parameter",
							 "-Wno-unused-function",
							 "-g", "-O0")

common_settings.link.exe = "i586-elf-gcc"
common_settings.link.extension = ".bin"
common_settings.link.flags:Add("-ffreestanding",
							   "-nostdlib",
							   "-O0")
common_settings.link.libs:Add("gcc")
common_settings.link.Output = BuildOutput

user_settings = TableDeepCopy(common_settings)
user_settings.cc.includes:Add('src/lib/include')

--
-- Require build scripts for all components
--

require 'src/common/libkogata/bam'
require 'src/common/libc/bam'
require 'src/common/libalgo/bam'

require 'src/kernel/bam'

require 'src/lib/libkogata/bam'

require 'src/sysbin/bam'

require 'res/fonts/bam'
require 'res/keymaps/bam'

--
-- Build script for CDROM.iso
--

cdrom = "build/cdrom.iso"

AddJob(cdrom, "building ISO", "./make_cdrom.sh")
AddDependency(cdrom, kernel, sysbin, fonts, keymaps)

DefaultTarget(cdrom)

--
-- Script for running tests
--

tests = {}

require 'src/tests/slab_test/bam'
require 'src/tests/ktests/bam'
require 'src/tests/utests/bam'

PseudoTarget("test", tests)
