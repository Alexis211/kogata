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

local fnt = draw.load_font('sys:/fonts/default.bf')

local mouse_fd = sys.open("io:/input/pcmouse", sysdef.FM_READ)
print("mouse_fd = " .. mouse_fd)

local cursor = draw.load_image('sys:/cursors/left_ptr.png')
local csrbkp = draw.new_surface(cursor:width(), cursor:height(), vesa_info.bpp, false)
csrbkp:blit(0, 0, surface:sub(0, 0, cursor:width(), cursor:height()))

local i = 1
local mouse_x, mouse_y = 0, 0
while true do
	local mouse_data, mouse_l = sys.read(mouse_fd, 0, 8)
	if mouse_l > 0 then
		print("mouse_l = ", mouse_l)
		print("mouse_data = ", string.unpack("hhbBBB", mouse_data))
		dx, dy = string.unpack("hhbBBB", mouse_data)

		surface:blit(mouse_x, mouse_y, csrbkp)

		mouse_x = mouse_x + dx
		mouse_y = mouse_y - dy
		if mouse_x < 0 then mouse_x = 0 end
		if mouse_y < 0 then mouse_y = 0 end
		if mouse_x >= vesa_info.width then mouse_x = vesa_info.width - 1 end
		if mouse_y >= vesa_info.height then mouse_y = vesa_info.height - 1 end

		csrbkp:blit(0, 0, surface:sub(mouse_x, mouse_y, cursor:width(), cursor:height()))
		surface:blit(mouse_x, mouse_y, cursor)
	end


	--[[
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
	--]]
end

os.exit()
