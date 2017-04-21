local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'
local draw = require 'lx.draw'

print("Hello, world!")

local vesa_fd = sys.open("io:/display/vesa", sysdef.FM_IOCTL | sysdef.FM_READ | sysdef.FM_WRITE | sysdef.FM_MMAP)
print("vesa_fd = " .. vesa_fd)

local vesa_info = ioctl.fb_get_info(vesa_fd)
print("vesa_info = ", vesa_info)

local surface = draw.from_fd(vesa_fd, vesa_info)
for x = 0, 255 do
	for y = 0, 255 do
		surface:plot(x, y, surface:rgb(x, y, 0))
	end
end

local i = 1
while true do
	surface:plot(i % (vesa_info.width-3),
				 i % (vesa_info.height-1),
				 surface:rgb(i % 256,
				 			 (i + 96) % 256,
				 			 (i + 2*96) % 256))
	i = i + 1
	if i % 100000 == 0 then print(i) end
end

os.exit()
