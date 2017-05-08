local sys = require 'lx.sys'
local sysdef= require 'lx.sysdef'
local ioctl = require 'lx.ioctl'
local draw = require 'lx.draw'

local gui = require 'lx.gui'
local mainloop = require 'lx.mainloop'

local tk = require 'lx.tk'

print("Hello, world!")

gui.open()
gui.load_cursor('sys:/cursors/left_ptr.png')
gui.show_cursor()

for x = 0, 255, 16 do
	for y = 0, 255, 16 do
		gui.surface:fillrect(x, y, 16, 16, gui.surface:rgb(x, y, 0))
	end
end

local f = draw.load_image('root:/logo.png')
local wm = tk.window_manager()
tk.init(gui, wm)

local img = tk.image(f)
wm:add(img, "Window 1", 100, 16)
local img2 = tk.box({
			center_content = true,
			vscroll = true,
			hscroll = true,
			vresize = true,
			hresize = true,
		}, tk.image(f))
wm:add(img2, "Window 2", 16, 100)
local img3 = tk.box({
			constrain_size = true,
			vresize = true,
			hresize = true,
		}, tk.image(f))
wm:add(img3, "Window 3", 300, 150)
local txt = tk.text({color = tk.rgb(0, 255, 0), width=200}, "Hello, world!\nThis is a long text")
wm:add(txt, "Text example", 32, 32)


mainloop.run()

os.exit()
