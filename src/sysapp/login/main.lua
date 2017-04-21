local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'

print("Hello, world!")

local vesa_fd = sys.open("io:/display/vesa", sysdef.FM_IOCTL | sysdef.FM_READ | sysdef.FM_WRITE | sysdef.FM_MMAP)
print("vesa_fd = " .. vesa_fd)

local vesa_info = ioctl.fb_get_info(vesa_fd)
print("vesa_info = ", vesa_info)

local i = 1
while true do
	print(i)
	i = i + 1
	sys.usleep(1000000)
end

os.exit()
