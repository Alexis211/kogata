local draw = require 'lx.draw'
local sys = require 'lx.sys'

local tk = {}

function tk.widget(width, height)
	local w = {
		width = width,
		height = height,
	}

	function w:resize(width, height)
		w.width = width
		w.height = height
	end

	function w:get_draw_buffer(x0, y0, w, h)
		-- Replaced by parent by a function that returns the buffer for
		return nil
	end

	function w:redraw(x0, y0, w, h)
		-- Replaced by widget code by a function that does the actual drawing
	end

	function w:on_mouse_down(lb, rb, mb)
		-- Handler for mouse down event
	end

	function w:on_mouse_up(lb, rb, mb)
		-- Handler for mouse up event
	end

	function w:on_mouse_move(prev_x, prev_y, new_x, new_y)
		-- Handler for mouse move event
	end

	function w:on_text_input(char)
		-- Handler for text input
	end

	function w:on_key_down(key)
		-- Handler for key press
	end

	function w:on_key_up(key)
		-- Handler for key release
	end

	return w
end

function tk.init(gui, root_widget)
	tk.fonts = {
		default = draw.load_ttf_font("sys:/fonts/vera.ttf")
	}

	root_widget:resize(gui.surface:width(), gui.surface:height())

	gui.on_key_down = function(key) root_widget:on_key_down(key) end
	gui.on_key_up = function(key) root_widget:on_key_up(key) end
	gui.on_text_input = function(key) root_widget:on_text_input(char) end
	gui.on_mouse_move = function(ox, oy, nx, ny) root_widget:on_mouse_move(ox, oy, nx, ny) end
	gui.on_mouse_down = function(lb, rb, mb) root_widget:on_mouse_down(lb, rb, mb) end
	gui.on_mouse_up = function(lb, rb, mb) root_widget:on_mouse_up(lb, rb, mb) end

	function root_widget:get_draw_buffer(x0, y0, w, h)
		return gui.surface:sub(x0, y0, w, h)
	end

	root_widget:redraw(0, 0, root_widget.width, root_widget.height)
end

function tk.image_widget(img)
	local image = tk.widget(img:width(), img:height())
	image.img = img

	function image:redraw(x0, y0, w, h)
		local buf = self:get_draw_buffer(x0, y0, w, h)
		if buf == nil then return end

		local step = 20
		local halfstep = 10
		for x = x0 - (x0 % step), x0 + w, step do
			for y = y0 - (y0 % step), y0 + h, step do
				buf:fillrect(x - x0, y - y0, halfstep, halfstep, buf:rgb(150, 150, 150))
				buf:fillrect(x - x0 + halfstep, y - y0 + halfstep, halfstep, halfstep, buf:rgb(150, 150, 150))
				buf:fillrect(x - x0 + halfstep, y - y0, halfstep, halfstep, buf:rgb(170, 170, 170))
				buf:fillrect(x - x0, y - y0 + halfstep, halfstep, halfstep, buf:rgb(170, 170, 170))
			end
		end
		buf:blit(0, 0, self.img:sub(x0, y0, self.img:width(), self.img:height()))
	end

	return image
end


return tk
