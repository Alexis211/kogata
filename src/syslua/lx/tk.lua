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

	function w:redraw(x0, y0, buf)
		-- Replaced by widget code by a function that does the actual drawing
	end

	function w:on_mouse_down(x, y, lb, rb, mb)
		-- Handler for mouse down event
	end

	function w:on_mouse_up(x, y, lb, rb, mb)
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
	if tk.root_widget ~= nil then return end
	tk.root_widget = root_widget

	if root_widget.parent ~= nil then return end
	root_widget.parent = gui
	
	tk.fonts = {
		default = draw.load_ttf_font("sys:/fonts/vera.ttf")
	}

	root_widget:resize(gui.surface:width(), gui.surface:height())

	gui.on_key_down = function(key) root_widget:on_key_down(key) end
	gui.on_key_up = function(key) root_widget:on_key_up(key) end
	gui.on_text_input = function(key) root_widget:on_text_input(char) end
	gui.on_mouse_move = function(ox, oy, nx, ny) root_widget:on_mouse_move(ox, oy, nx, ny) end
	gui.on_mouse_down = function(lb, rb, mb) root_widget:on_mouse_down(gui.mouse_x, gui.mouse_y, lb, rb, mb) end
	gui.on_mouse_up = function(lb, rb, mb) root_widget:on_mouse_up(gui.mouse_x, gui.mouse_y, lb, rb, mb) end

	function root_widget:get_draw_buffer(x0, y0, w, h)
		return gui.surface:sub(x0, y0, w, h)
	end

	root_widget:redraw(0, 0, gui.surface)
end

function tk.image_widget(img)
	local image = tk.widget(img:width(), img:height())
	image.img = img

	function image:redraw(x0, y0, buf)
		local step = 20
		local halfstep = 10
		for x = x0 - (x0 % step), x0 + buf:width(), step do
			for y = y0 - (y0 % step), y0 + buf:height(), step do
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

function tk.wm_widget()
	local wm = tk.widget(100, 100)
	wm.windows = {}		-- Last = on top

	function wm:add(content, title, x, y)
		local win = tk.widget(content.width + 2, content.height + 22)
		win.parent = self
		win.x = x or 24
		win.y = y or 24
		win.title = title
		win.visible = true
		table.insert(self.windows, win)

		win.content = content
		content.parent = win
		content.x = 1
		content.y = 21

		function content:get_draw_buffer(x0, y0, w, h)
			if x0 + w > self.width then w = self.width - x0 end
			if y0 + h > self.height then h = self.height - y0 end
			return win:get_draw_buffer(x0 + 1, y0 + 21, w, h)
		end

		function win:get_draw_buffer(x0, y0, w, h)
			-- TODO clipping etc
		end

		function win:redraw(x0, y0, buf)
			local xx0 = math.max(x0, 1)
			local yy0 = math.max(y0, 21)

			local ww, hh = buf:width() - (xx0-x0), buf:height() - (yy0-y0)
			if xx0-1 + ww > content.width then ww = content.width - xx0+1 end
			if yy0-21 + ww > content.height then hh = content.height - yy0+21 end

			self.content:redraw(xx0-1, yy0-21, buf:sub(xx0-x0, yy0-y0, ww, hh))

			buf:rect(-x0, -y0, self.width, self.height, buf:rgb(255, 128, 0))
			buf:fillrect(-x0+1, -y0+1, win.width-2, 20, buf:rgb(200, 200, 200))
			if win.title then
				buf:write(-x0+2, -y0+2, win.title, tk.fonts.default, 16, buf:rgb(0, 0, 0))
			end
		end

		function win:on_mouse_down(x, y, lb, rb, mb)
			if y < 22 and lb then
				self.moving = true
			else
				self.content:on_mouse_down(x-1, y-21, lb, rb, mb)
			end
		end

		function win:on_mouse_up(x, y, lb, rb, mb)
			if self.moving then
				self.moving = false
			else
				self.content:on_mouse_up(x-1, y-21, lb, rb, mb)
			end
		end

		function win:on_mouse_move(px, py, nx, ny)
			if self.moving then
				self.visible = false
				wm:redraw_win(self)
				self.x = self.x + nx - px
				self.y = self.y + ny - py
				self.visible = true
				wm:redraw_win(self)
			else
				self.content:on_mouse_move(px-1, py-21, px-1, py-21)
			end
		end

		self:redraw_win(win)
	end

	function wm:remove(win)
		if win.parent ~= self then return end
		win.parent = nil
		win.get_draw_buffer = function() return nil end

		win.content.parent = nil
		win.content.get_draw_buffer = function() return nil end

		for i, w2 in pairs(self.windows) do
			if w2 == win then
				table.remove(self.windows, i)
				return
			end
		end
	end

	function wm:redraw_win(win)
		local x0, y0 = math.max(0, win.x), math.max(0, win.y)
		local width = win.x + win.width - x0
		local height = win.y + win.height - y0
		self:redraw(x0, y0, self:get_draw_buffer(x0, y0, width, height))
	end

	function wm:redraw(x0, y0, buf)
		-- Draw background

		local step = 32
		local halfstep = 16
		for x = x0 - (x0 % step), x0 + buf:width(), step do
			for y = y0 - (y0 % step), y0 + buf:height(), step do
				buf:fillrect(x - x0, y - y0, halfstep, halfstep, buf:rgb(110, 110, 140))
				buf:fillrect(x - x0 + halfstep, y - y0 + halfstep, halfstep, halfstep, buf:rgb(110, 110, 140))
				buf:fillrect(x - x0 + halfstep, y - y0, halfstep, halfstep, buf:rgb(110, 140, 110))
				buf:fillrect(x - x0, y - y0 + halfstep, halfstep, halfstep, buf:rgb(110, 140, 110))
			end
		end

		-- TODO do this in inverse order and clip regions of hidden windows
		-- so that less redrawing is done
		for i = 1, #self.windows do
			win = self.windows[i]
			if win.visible then
				local wx0, wy0 = math.max(x0, win.x), math.max(y0, win.y)
				local ww, wh = win.x + win.width - wx0, win.y + win.height - wy0
				if ww > 0 and wh > 0 then
					win:redraw(wx0 - win.x, wy0 - win.y, buf:sub(wx0 - x0, wy0 - y0, ww, wh))
				end
			end
		end
	end

	function wm:find_win(x, y)
		for i = #self.windows, 1, -1 do
			local win = self.windows[i]
			if win.visible and x >= win.x and y >= win.y and x < win.x + win.width and y < win.y + win.height then
				return win
			end
		end
		return nil
	end

	function wm:on_mouse_down(x, y, lb, rb, mb)
		local on_win = self:find_win(x, y)
		if on_win then
			sys.dbg_print(string.format("Mouse down on window %s\n", on_win.title))

			if self.windows[#self.windows] ~= on_win then
				for i, w in pairs(self.windows) do
					if w == on_win then
						table.remove(self.windows, i)
						break
					end
				end
				table.insert(self.windows, on_win)
				self:redraw_win(on_win)
			end
			on_win:on_mouse_down(x - on_win.x, y - on_win.y, lb, rb, mb)
		end
	end

	function wm:on_mouse_up(x, y, lb, rb, mb)
		local on_win = self:find_win(x, y)
		if on_win then
			on_win:on_mouse_up(x - on_win.x, y - on_win.y, lb, rb, mb)
		end
	end

	function wm:on_mouse_move(prev_x, prev_y, new_x, new_y)
		local on_win = self:find_win(prev_x, prev_y)
		if on_win then
			on_win:on_mouse_move(prev_x - on_win.x, prev_y - on_win.y, new_x - on_win.x, new_y - on_win.y)
		end
	end

	return wm
end


return tk
