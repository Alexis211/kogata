local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'
local draw = require 'lx.draw'

local gui = require 'lx.gui'
local mainloop = require 'lx.mainloop'

print("Hello, world!")

gui.open()
gui.load_cursor('sys:/cursors/left_ptr.png')

for x = 0, 255, 16 do
	for y = 0, 255, 16 do
		gui.surface:fillrect(x, y, 16, 16, gui.surface:rgb(x, y, 0))
	end
end

gui.show_cursor()

local fnt = draw.load_font('sys:/fonts/default.bf')

local txt_x, txt_y = 0, 0

gui.on_mouse_down = function (lb, rb, mb)
	if lb then
		gui.hide_cursor()
		gui.surface:write(gui.mouse_x, gui.mouse_y, string.format("Click at %d, %d", gui.mouse_x, gui.mouse_y), fnt, gui.surface:rgb(0, 0, 255))
		txt_x = gui.mouse_x
		txt_y = gui.mouse_y + fnt:text_height(" ")
		gui.show_cursor()
	end
end

gui.on_text_input = function(chr)
	gui.hide_cursor()
	gui.surface:write(txt_x, txt_y, chr, fnt, gui.surface:rgb(0, 0, 255))
	txt_x = txt_x + fnt:text_width(chr)
	gui.show_cursor()
end

mainloop.run()

os.exit()
