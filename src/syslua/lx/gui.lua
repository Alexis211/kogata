local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'
local draw = require 'lx.draw'

local mainloop = require 'lx.mainloop'

local gui = {
	-- Keyboard info
	-- TODO
	-- Mouse info
	mouse_x = 0,
	mouse_y = 0,
	mouse_lbtn = false,
	mouse_rbtn = false,
	mouse_midbtn = false,
}

function gui.open()
	if sys.stat('io:/display/vesa') and sys.stat('io:/input/pckbd') and sys.stat('io:/input/pcmouse') then
		gui.open_io()
	elseif sys.stat_open(sysdef.STD_FD_GIP) then
		gui.open_gip()
	else
		error("Could not open GUI!")
	end
end

function gui.open_io()
	-- Open display
	gui.vesa_fd = sys.open("io:/display/vesa", sysdef.FM_IOCTL | sysdef.FM_READ | sysdef.FM_WRITE | sysdef.FM_MMAP)
	assert(gui.vesa_fd ~= 0)

	gui.vesa_info = ioctl.fb_get_info(gui.vesa_fd)
	gui.surface = draw.surface_from_fd(gui.vesa_fd, gui.vesa_info)

	-- Open keyboard
	gui.pckbd_fd = sys.open("io:/input/pckbd", sysdef.FM_READ)
	assert(gui.pckbd_fd ~= 0)

	gui.pckbd_mainloop_fd = mainloop.add_fd(gui.pckbd_fd, function() error('pckbd fd error') end)
	local function pckbd_handler(ev)
		local scancode, ty = string.unpack("HH", ev)
		gui.on_keyboard(scancode, ty)

		gui.pckbd_mainloop_fd:expect(4, pckbd_handler)
	end
	gui.pckbd_mainloop_fd:expect(4, pckbd_handler)

	-- Open mouse
	gui.pcmouse_fd = sys.open("io:/input/pcmouse", sysdef.FM_READ)
	assert(gui.pcmouse_fd ~= 0)

	gui.pcmouse_mainloop_fd = mainloop.add_fd(gui.pcmouse_fd, function() error("pcmouse fd error") end)
	local function pcmouse_handler(ev)
		local dx, dy, dw, lb, rb, mb = string.unpack("hhbBBB", ev)
		gui.on_mouse(dx, dy, dw, lb, rb, mb)

		gui.pcmouse_mainloop_fd:expect(8, pcmouse_handler)
	end
	gui.pcmouse_mainloop_fd:expect(8, pcmouse_handler)
end

function gui.open_gip()
	-- TODO
end

function gui.on_keyboard(scancode, ty)
	-- TODO
end

function gui.on_mouse(dx, dy, dw, lb, rb, mb)
	-- TODO
end

return gui
