local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'
local draw = require 'lx.draw'

local kbd = require 'lx.kbd'
local kbdcode = require 'lx.kbdcode'

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
	cursor_visible = false,
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

	gui.surface_geom = ioctl.fb_get_info(gui.vesa_fd)
	gui.surface = draw.surface_from_fd(gui.vesa_fd, gui.surface_geom)

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

	gui.kbdlib = kbd.init()

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

function gui.load_cursor(filename)
	gui.hide_cursor()
	gui.cursor = draw.load_image(filename)
	gui.cursor_backup = draw.new_surface(gui.cursor:width(),gui.cursor:height(), gui.surface_geom.bpp, false)
end

function gui.hide_cursor()
	if not gui.cursor_visible then return end

	gui.surface:blit(gui.mouse_x, gui.mouse_y, gui.cursor_backup)
	gui.cursor_visible = false
end

function gui.show_cursor()
	if gui.cursor_visible then return end
	if not gui.cursor then return end
	
	gui.cursor_backup:blit(0, 0, gui.surface:sub(gui.mouse_x, gui.mouse_y, gui.cursor:width(), gui.cursor:height()))
	gui.surface:blit(gui.mouse_x, gui.mouse_y, gui.cursor)
	gui.cursor_visible = true
end

function gui.on_keyboard(scancode, ty)
	if ty == kbdcode.event.KEYPRESS then
		local key = gui.kbdlib:press(scancode)
		gui.on_key_down(key)
		if key.chr then
			gui.on_text_input(string.char(key.chr))
		end
	elseif ty == kbdcode.event.KEYRELEASE then
		local key = gui.kbdlib:release(scancode)
		gui.on_key_up(key)
	end
end

function gui.on_key_down(key)
	-- Can be replaced :)
end

function gui.on_text_input(chr)
	-- Can be replaced :)
end

function gui.on_key_up(key)
	-- Can be replaced :)
end


function gui.on_mouse(dx, dy, dw, lb, rb, mb)
	if dx ~= 0 or dy ~= 0 then
		local prev_x, prev_y = gui.mouse_x, gui.mouse_y

		local csr = gui.cursor_visible
		if csr then gui.hide_cursor() end

		gui.mouse_x = gui.mouse_x + dx
		gui.mouse_y = gui.mouse_y - dy
		if gui.mouse_x < 0 then gui.mouse_x = 0 end
		if gui.mouse_y < 0 then gui.mouse_y = 0 end
		if gui.mouse_x >= gui.surface:width() then gui.mouse_x = gui.surface:width() - 1 end
		if gui.mouse_y >= gui.surface:height() then gui.mouse_y = gui.surface:height() - 1 end

		if csr then gui.show_cursor() end

		gui.on_mouse_move(prev_x, prev_y, gui.mouse_x, gui.mouse_y)
	end

	if lb == 1 and not gui.mouse_lbtn then
		gui.mouse_lbtn = true
		gui.on_mouse_down(true, false, false)
	elseif lb == 0 and gui.mouse_lbtn then
		gui.mouse_lbtn = false
		gui.on_mouse_up(true, false, false)
	end

	if rb == 1 and not gui.mouse_rbtn then
		gui.mouse_rbtn = true
		gui.on_mouse_down(false, true, false)
	elseif rb == 0 and gui.mouse_rbtn then
		gui.mouse_rbtn = false
		gui.on_mouse_up(false, true, false)
	end
end

function gui.on_mouse_move(prev_x, prev_y, x, y)
	-- Nothing, can be replaced :)
end

function gui.on_mouse_down(lb, rb, mb)
	-- Nothing, can be replaced :)
end

function gui.on_mouse_up(lb, rb, mb)
	-- Nothing, can be replaced :)
end

return gui
