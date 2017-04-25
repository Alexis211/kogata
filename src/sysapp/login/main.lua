local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'
local draw = require 'lx.draw'

print("Hello, world!")

local vesa_fd = sys.open("io:/display/vesa", sysdef.FM_IOCTL | sysdef.FM_READ | sysdef.FM_WRITE | sysdef.FM_MMAP)
print("vesa_fd = " .. vesa_fd)

local vesa_info = ioctl.fb_get_info(vesa_fd)
print("vesa_info = ", vesa_info)

local surface = draw.surface_from_fd(vesa_fd, vesa_info)
for x = 0, 255 do
	for y = 0, 255 do
		surface:plot(x, y, surface:rgb(x, y, 0))
	end
end

local fnt = draw.load_font('default')

local mouse_fd = sys.open("io:/input/pcmouse", sysdef.FM_READ)
print("mouse_fd = " .. mouse_fd)

local i = 1
while true do
	local mouse_data, mouse_l = sys.read(mouse_fd, 0, 8)
	if mouse_l > 0 then
		print("mouse_l = ", mouse_l)
		print("mouse_data = ", string.unpack("hhbBBB", mouse_data))
	end

	surface:rect((i*3) % (vesa_info.width-3),
				 (i*3) % (vesa_info.height-5),
				 3, 3,
				 surface:rgb(i % 256,
				 			 (i + 96) % 256,
				 			 (i + 2*96) % 256))
	i = i + 1
	if i % 10000 == 0 then
		local x0 = math.random(vesa_info.width)-1
		local x1 = math.random(vesa_info.width)-1
		local y0 = math.random(vesa_info.height)-1
		local y1 = math.random(vesa_info.height)-1
		if x0 > x1 then x0, x1 = x1, x0 end
		if y0 > y1 then y0, y1 = y1, y0 end
		local c = surface:rgb(math.random(0, 255), math.random(0, 255), math.random(0, 255))
		surface:fillrect(x0, y0, x1-x0, y1-y0, c)
	end
	if i % 10000 == 0 then
		-- print(i)
		local txt = tostring(i)
		surface:fillrect(0, 0, fnt:text_width(txt),
							   fnt:text_height(txt),
							   surface:rgb(0, 0, 0))
		surface:write(0, 0, txt, fnt, surface:rgb(255, 0, 0))
	end
end

os.exit()
