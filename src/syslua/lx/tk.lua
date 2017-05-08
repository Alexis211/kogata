local draw = require 'lx.draw'
local sys = require 'lx.sys'

local tk = {}

--[[

	Widgets:

	window manager widget
	Window manager
	Widows are any widget which determine their own size

	box widget
	A box that can be:
	- scrollable (H/V)
	- resizable (H/V)
	Has a single child. Can either:
	a. constrain the size of the child to the size of the box
	b. let the child widget determine its size
	Can have a margin (outer margin) with associated outer background color,
	         a padding (inner margin) with associated background color,
			 a border with its color
	Can have onclick events to implement buttons.

	grid widget
	A grid widget, organizes its children in a grid, which can make a table or any kind of layout. It:
	- resizes its childs and repositions them to make a grid
	- accepts when its childs are resized and changes the width of columns
	  or the height of lines accordingly
	- draws borders/spacing between elements

	text widget

	image widget

	border widget

	centered widget

	text input

	text box



	text_layout_widget
	Positions text widgets and images widgets in a subtle and intelligent manner (lol)


How to make a table:

	widget = tk.grid({},
		{ { tk.box({h_resizable = true, width = 200}, tk.text("name")),
		    tk.box({h_resizable = true, width = 100}, tk.text("actions")) },
		  { tk.box({padding = 4}, tk.text("file A")),
		    tk.grid({}, {{ tk.box({padding = 4, border = 1, border_color = red,
			 			   on_click = function() do something end}, tk.text("open")) }}) } }

--]]

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



-- ACTUAL TK STUFF


function tk.widget(width, height, opts)
	local w = {
		x = 0,
		y = 0,
		width = width,
		height = height,
	}
	if opts then
		for k, v in pairs(opts) do
			w[k] = v
		end
	end

	function w:resize(width, height)
		if width then w.width = width end
		if width then w.height = height end
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
		if lb then
		-- Handler for mouse down event
			self.left_click_valid = true
		end
	end

	function w:on_mouse_up(x, y, lb, rb, mb)
		-- Handler for mouse up event
		if lb and self.left_click_valid then
			self:on_click()
		end
	end

	function w:on_mouse_move(prev_x, prev_y, new_x, new_y)
		-- Handler for mouse move event
		self.left_click_valid = false
	end

	function w:on_click()
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
		["vera.ttf"] = draw.load_ttf_font("sys:/fonts/vera.ttf"),
		["veraserif.ttf"] = draw.load_ttf_font("sys:/fonts/veraserif.ttf"),
		["veramono.ttf"] = draw.load_ttf_font("sys:/fonts/veramono.ttf"),
	}
	tk.fonts.default = tk.fonts["vera.ttf"]

	function tk.rgb(r, g, b)
		return gui.surface:rgb(r, g, b)
	end

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

	root_widget:redraw(0, 0, root_widget:get_draw_buffer(0, 0, root_widget.width, root_widget.height))
	root_widget:finalize_draw_buffer()
end

-- #### #### STANDARD WIDGETS #### ####

function tk.image(a, b)
	local img = b or a
	local opts = b and a or {}

	local image = tk.widget(img:width(), img:height(), opts)
	image.img = img

	function image:resize(width, height)
		if width then 
			self.width = width
			if self.width < self.img:width() then self.width = self.img:width() end
		end
		if height then
			self.height = height
			if self.height < self.img:height() then self.height = self.img:height() end
		end
	end

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
		if x0 < self.img:width() and y0 < self.img:height() then
			buf:blit(0, 0, self.img:sub(x0, y0, self.img:width(), self.img:height()))
		end
	end

	return image
end

function tk.text(a, b)
	local text = b or a
	local opts = b and a or {}

	-- Some defaults
	opts.text_size = opts.text_size or 16
	opts.padding = opts.padding or 2
	opts.background = opts.background or tk.rgb(255, 255, 255)
	opts.color = opts.color or tk.rgb(0, 0, 0)
	opts.line_spacing = opts.line_spacing or 1
	if opts.word_wrap == nil then opts.word_wrap = true end
	opts.font = opts.font or tk.fonts.default
	opts.dy = opts.dy or -(opts.text_size//8)

	local txt = tk.widget(100, #string.split(text, "\n")*(opts.text_size + opts.line_spacing) - opts.line_spacing + 2*opts.padding, opts)
	txt.text = text

	function txt:redraw(x0, y0, buf)
		local lines = string.split(self.text, "\n")

		buf:fillrect(0, 0, buf:width(), buf:height(), self.background)
		local acceptable_surface = region_inter({x = x0, y = y0, w = buf:width(), h = buf:height()},
												{x = self.padding, y = self.padding, w = self.width - 2*self.padding, h = self.height - 2*self.padding})
		for _, s in pairs(acceptable_surface) do
			local buf2 = buf:sub(s.x - x0, s.y - y0, s.w, s.h)
			for i = 1, #lines do
				-- TODO word wrap
				buf2:write(self.padding - s.x, (i-1) * (self.text_size + self.line_spacing) + self.padding - s.y + self.dy,
						   lines[i], self.font, self.text_size, self.color)
			end
		end
	end
	
	return txt
end

function tk.box(a, b)
	local content = b or a
	local opts = b and a or {}

	-- Some defaults
	opts.hresize = opts.hresize or false
	opts.vresize = opts.vresize or false
	opts.hscroll = opts.hscroll or false
	opts.vscroll = opts.vscroll or false
	opts.center_content = opts.center_content or false
	opts.constrain_size = opts.constrain_size or nil
	opts.background_color = opts.background_color or tk.rgb(190, 190, 190)
	opts.control_size = 12

	local box = tk.widget(content.width, content.height, opts)
	box.content = content
	if box.center_content then
		if box.width > box.content.width then
			box.content.x = (box.width - box.content.width) // 2
		else
			box.content.x = 0
		end
		if box.height > box.content.height then
			box.content.y = (box.height - box.content.height) // 2
		else
			box.content.y = 0
		end
	else
		box.content.x = 0
		box.content.y = 0
	end

	function box:resize(width, height)
		local ow, oh = self.width, self.height
		if self.constrain_size then
			self.content:resize(width, height)
			self.width = content.width
			self.height = content.height
		else
			if width then self.width = width end
			if height then self.height = height end
			if self.center_content then
				if self.width > self.content.width then
					self.content.x = (self.width - self.content.width) // 2
				else
					self.content.x = math.min(0, math.max(-(self.content.width - self.width), self.content.x))
				end
				if self.height > self.content.height then
					self.content.y = (self.height - self.content.height) // 2
				else
					self.content.y = math.min(0, math.max(-(self.content.height - self.height), self.content.y))
				end
			end
		end
		if self.parent and (self.width ~= ow or self.height ~= oh) then
			self.parent:child_resized(self)
		end
	end

	function box:move_content(x, y)
		local ox, oy = self.content.x, self.content.y
		if x then self.content.x = math.min(0, math.max(-(self.content.width - self.width), x)) end
		if y then self.content.y = math.min(0, math.max(-(self.content.height - self.height), y)) end
		if self.parent and (self.content.x ~= ox or self.content.y ~= oy) then
			self.parent:child_resized(self)
		end
	end

	function box:barcalc(pos, insize, outsize)
		local csz = self.control_size
		local barsize = (outsize - csz) * outsize // insize
		local baravail = outsize - csz - barsize
		local barpos = -pos * baravail // (insize - outsize)
		return barsize, barpos, baravail
	end

	function box:redraw(x0, y0, buf)
		local csz = self.control_size

		local acceptable_surface = region_inter({x = x0, y = y0, w = buf:width(), h = buf:height()},
												{x = self.content.x, y = self.content.y,
												 w = self.content.width, h = self.content.height})
		for _, s in pairs(acceptable_surface) do
			local buf2 = buf:sub(s.x - x0, s.y - y0, s.w, s.h)
			self.content:redraw(s.x - self.content.x, s.y - self.content.y, buf2)
		end
		local background_surface = region_diff({x = x0, y = y0, w = buf:width(), h = buf:height()},
											   {x = self.content.x, y = self.content.y,
												w = self.content.width, h = self.content.height})
		for _, s in pairs(background_surface) do
			buf:fillrect(s.x - x0, s.y - y0, s.w, s.h, self.background_color)
		end

		if self.vresize or self.hresize then
			buf:rect(self.width - csz - x0, self.height - csz - y0, csz, csz, buf:rgb(0, 0, 0))
			buf:fillrect(self.width - csz + 1 - x0, self.height - csz + 1 - y0, csz - 2, csz - 2, buf:rgb(255, 255, 255))
		end
		if self.hscroll and self.width < self.content.width then
			local barsize, barpos = self:barcalc(self.content.x, self.content.width, self.width)
			buf:fillrect(barpos + 4 - x0, self.height - csz + 2 - y0, barsize - 8, csz - 4, buf:rgb(0, 0, 0))
		end
		if self.vscroll and self.height < self.content.height then
			local barsize, barpos = self:barcalc(self.content.y, self.content.height, self.height)
			buf:fillrect(self.width - csz + 2 - x0, barpos + 4 - y0, csz - 4, barsize - 8, buf:rgb(0, 0, 0))
		end
	end

	function box:on_mouse_down(x, y, lb, mb, rb)
		local csz = self.control_size

		if lb and (self.vresize or self.hresize) and x >= self.width - csz and y >= self.height - csz then
			self.resizing = true
			self.resize_initw = self.width
			self.resize_inith = self.height
			self.resize_initmx = x
			self.resize_initmy = y
		elseif lb and self.vscroll and x >= self.width - csz and self.height < self.content.height then
			local barsize, barpos = self:barcalc(self.content.y, self.content.height, self.height)
			if y < barpos then
				self:move_content(nil, self.content.y + self.height * 2 // 3)
			elseif y >= barpos + barsize then
				self:move_content(nil, self.content.y - self.height * 2 // 3)
			else
				self.vscrolling = true
				self.vscrolling_inity = self.content.y
				self.vscrolling_initmy = y
			end
		elseif lb and self.hscroll and y >= self.height - csz and self.width < self.content.width then
			local barsize, barpos = self:barcalc(self.content.x, self.content.width, self.width)
			if x < barpos then
				self:move_content(self.content.x + self.width * 2 // 3, nil)
			elseif x >= barpos + barsize then
				self:move_content(self.content.x - self.width * 2 // 3, nil)
			else
				self.hscrolling = true
				self.hscrolling_initx = self.content.x
				self.hscrolling_initmx = x
			end
		else
			self.content:on_mouse_down(x - self.content.x, y - self.content.y, lb, mb, rb)
		end
	end

	function box:on_mouse_up(x, y, lb, mb, rb)
		if lb and self.resizing then
			self.resizing = false
		elseif lb and self.vscrolling then
			self.vscrolling = false
		elseif lb and self.hscrolling then
			self.hscrolling = false
		else
			self.content:on_mouse_up(x - self.content.x, y - self.content.y, lb, mb, rb)
		end
	end

	function box:on_mouse_move(px, py, nx, ny)
		if self.resizing then
			local w = self.hresize and self.resize_initw + nx - self.resize_initmx
			local h = self.vresize and self.resize_inith + ny - self.resize_initmy
			self:resize(w, h)
		elseif self.vscrolling then
			local barsize, barpos, baravail = self:barcalc(self.content.y, self.content.height, self.height)
			self:move_content(nil, self.vscrolling_inity - (ny - self.vscrolling_initmy) * (self.content.height - self.height) // baravail)
		elseif self.hscrolling then
			local barsize, barpos, baravail = self:barcalc(self.content.x, self.content.width, self.width)
			self:move_content(self.hscrolling_initx - (nx - self.hscrolling_initmx) * (self.content.width - self.width) // baravail)
		else
			self.content:on_mouse_move(px - self.content.x, py - self.content.y, nx - self.content.x, ny - self.content.y)
		end
	end


	return box
end


-- #### #### WINDOW MANAGER WIDGET #### ####

function tk.window_manager()
	local wm = tk.widget(100, 100)
	wm.windows = {}		-- Last = on top
	wm.window_pos = {}

	wm.mouse_lb = false
	wm.mouse_rb = false
	wm.mouse_mb = false
	wm.mouse_win = nil

	function wm:add(content, title, x, y)
		local win = tk.widget(content.width + 2, content.height + 22)
		win.parent = self
		win.x = x or 24
		win.y = y or 24
		win.title = title
		win.visible = true
		table.insert(self.windows, win)
		self.window_pos[win] = {x = win.x, y = win.y, h = win.height, w = win.width}

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

		function win:child_resized(c)
			assert(c == self.content)

			local reg1 = wm.window_pos[self]
			self.width = c.width + 2
			self.height = c.height + 22
			wm.window_pos[self] = {x = self.x, y = self.y, w = self.width, h = self.height}
			local reg2 = wm.window_pos[self]
			local pieces = region_union(reg1, reg2)
			for _, p in pairs(pieces) do
				wm:redraw_region(p.x, p.y, p.w, p.h)
			end
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
				local reg1 = wm.window_pos[self]
				self.x = self.x + nx - px
				self.y = self.y + ny - py
				wm.window_pos[self] = {x = self.x, y = self.y, w = self.width, h = self.height}
				local reg2 = wm.window_pos[self]
				local pieces = region_union(reg1, reg2)
				for _, p in pairs(pieces) do
					wm:redraw_region(p.x, p.y, p.w, p.h)
				end
			else
				self.content:on_mouse_move(px-1, py-21, nx-1, ny-21)
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
		self.mouse_lb = self.mouse_lb or lb
		self.mouse_rb = self.mouse_rb or rb
		self.mouse_mb = self.mouse_mb or mb

		local on_win = self:find_win(x, y)
		if self.mouse_win then
			on_win = self.mouse_win
		else
			self.mouse_win = on_win
		end

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
		local on_win = self.mouse_win or self:find_win(x, y)
		if on_win then
			on_win:on_mouse_up(x - on_win.x, y - on_win.y, lb, rb, mb)
		end
		self.mouse_lb = self.mouse_lb and not lb
		self.mouse_rb = self.mouse_rb and not rb
		self.mouse_mb = self.mouse_mb and not mb
		if not (self.mouse_lb or self.mouse_mb or self.mouse_rb) then
			self.mouse_win = nil
		end
	end

	function wm:on_mouse_move(prev_x, prev_y, new_x, new_y)
		local on_win = self.mouse_win or self:find_win(prev_x, prev_y)
		if on_win then
			on_win:on_mouse_move(prev_x - on_win.x, prev_y - on_win.y, new_x - on_win.x, new_y - on_win.y)
		end
	end

	return wm
end


return tk
