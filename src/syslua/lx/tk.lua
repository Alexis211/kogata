local draw = require 'lx.draw'
local sys = require 'lx.sys'

local tk = {}

-- UTIL

function region_operation(reg1, reg2, sel_fun)
	local xs = {reg1.x, reg1.x + reg1.w, reg2.x, reg2.x + reg2.w}
	local ys = {reg1.y, reg1.y + reg1.h, reg2.y, reg2.y + reg2.h}
	table.sort(xs)
	table.sort(ys)
	local pieces = {}
	local function add_region(x, y, w, h)
		for i, r in pairs(pieces) do
			if r.y == y and r.h == h and r.x + r.w == x then
				table.remove(pieces, i)
				add_region(r.x, r.y, r.w + w, r.h)
				return
			elseif r.x == x and r.w == w and r.y + r.h == y then
				table.remove(pieces, i)
				add_region(r.x, r.y, r.w, r.h + h)
				return
			end
		end
		table.insert(pieces, {x = x, y = y, w = w, h = h})
	end
	for i = 1, 3 do
		for j = 1, 3 do
			local x0, x1, y0, y1 = xs[i], xs[i+1], ys[j], ys[j+1]
			if x1 > x0 and y1 > y0 then
				if sel_fun(x0, x1, y0, y1) then
					add_region(x0, y0, x1 - x0, y1 - y0)
				end
			end
		end
	end
	return pieces
end

function region_diff(reg1, reg2)
	return region_operation(reg1, reg2,
		function(x0, x1, y0, y1)
			return (x0 >= reg1.x and x0 < reg1.x + reg1.w and y0 >= reg1.y and y0 < reg1.y + reg1.h)
				and not (x0 >= reg2.x and x0 < reg2.x + reg2.w and y0 >= reg2.y and y0 < reg2.y + reg2.h) 
		end
	)
end

function region_inter(reg1, reg2)
	return region_operation(reg1, reg2,
		function(x0, x1, y0, y1)
			return (x0 >= reg1.x and x0 < reg1.x + reg1.w and y0 >= reg1.y and y0 < reg1.y + reg1.h) 
				and (x0 >= reg2.x and x0 < reg2.x + reg2.w and y0 >= reg2.y and y0 < reg2.y + reg2.h)
		end
	)
end

function region_union(reg1, reg2)
	return region_operation(reg1, reg2,
		function(x0, x1, y0, y1)
			return (x0 >= reg1.x and x0 < reg1.x + reg1.w and y0 >= reg1.y and y0 < reg1.y + reg1.h) 
				or (x0 >= reg2.x and x0 < reg2.x + reg2.w and y0 >= reg2.y and y0 < reg2.y + reg2.h)
		end
	)
end



function tk.widget(width, height)
	local w = {
		x = 0,
		y = 0,
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
	function w:finalize_draw_buffer()
		if self.parent ~= nil then
			self.parent:finalize_draw_buffer()
		end
	end

	function w:redraw(x0, y0, buf)
		-- Replaced by widget code by a function that does the actual drawing
	end

	function w:redraw_sub(x0, y0, buf, subwidget)
		local region = {x = x0, y = y0, w = buf:width(), h = buf:height()}
		local subregion = {x = subwidget.x, y = subwidget.y, w = subwidget.width, h = subwidget.height}
		local inter = region_inter(region, subregion)
		for _, z in pairs(inter) do
			subwidget:redraw(z.x - subwidget.x, z.y - subwidget.y, buf:sub(z.x - x0, z.y - y0, z.w, z.h))
		end
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
		if gui.cursor_visible and #region_inter({x=x0, y=y0, w=w, h=h},
												{x=gui.mouse_x, y=gui.mouse_y, w=gui.cursor:width(), h=gui.cursor:height()})>0
		then
			tk.must_reshow_cursor_on_finalize = true
			gui.hide_cursor()
		end
		return gui.surface:sub(x0, y0, w, h)
	end
	function root_widget:finalize_draw_buffer()
		if tk.must_reshow_cursor_on_finalize then
			tk.must_reshow_cursor_on_finalize = false
			gui.show_cursor()
		end
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
			self:redraw_sub(x0, y0, buf, self.content)

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
				local reg1 = {x = self.x, y = self.y, w = self.width, h = self.height}
				self.x = self.x + nx - px
				self.y = self.y + ny - py
				local reg2 = {x = self.x, y = self.y, w = self.width, h = self.height}
				local pieces = region_union(reg1, reg2)
				for _, p in pairs(pieces) do
					wm:redraw_region(p.x, p.y, p.w, p.h)
				end
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

	function wm:redraw_region(x, y, w, h)
		local ok = region_inter({x = 0, y = 0, w = self.width, h = self.height},
								{x = x, y = y, w = w, h = h})
		for _, r in pairs(ok) do
			self:redraw(r.x, r.y, self:get_draw_buffer(r.x, r.y, r.w, r.h))
			self:finalize_draw_buffer()
		end
	end

	function wm:redraw_win(win)
		self:redraw_region(win.x, win.y, win.width, win.height)
	end

	function wm:redraw(x0, y0, buf)
		local remaining = { {x = x0, y = y0, w = buf:width(), h = buf:height()} }

		-- TODO do this in inverse order and clip regions of hidden windows
		-- so that less redrawing is done
		for i = #self.windows, 1, -1 do
			win = self.windows[i]
			if win.visible then
				local remaining2 = {}
				local win_reg = {x = win.x, y = win.y, w = win.width, h = win.height}

				for _, reg in pairs(remaining) do
					local draw_to = region_inter(reg, win_reg)
					for _, reg2 in pairs(draw_to) do
						win:redraw(reg2.x - win.x, reg2.y - win.y, buf:sub(reg2.x - x0, reg2.y - y0, reg2.w, reg2.h))
					end

					local remain_to = region_diff(reg, win_reg)
					for _, reg2 in pairs(remain_to) do
						table.insert(remaining2, reg2)
					end
				end

				remaining = remaining2
			end
		end

		-- Draw background
		function draw_background(x0, y0, buf)
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
		end
		for _, reg in pairs(remaining) do
			draw_background(reg.x, reg.y, buf:sub(reg.x - x0, reg.y - y0, reg.w, reg.h))
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
