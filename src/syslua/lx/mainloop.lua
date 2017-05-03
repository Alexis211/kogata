local sys = require 'lx.sys'
local sysdef = require 'lx.sysdef'

local mainloop = {}

local fds = {}
local mainloop_must_exit = false
local mainloop_fds_change = false

function new_fd(fd, error_cb)
	local fd = {
		fd = fd,
		error_cb = error_cb,
		rd_expect = {},
		wr_buf = "",
	}
	function fd:write(str)
		-- TODO: try write immediately when possible
		self.wr_buf = self.wr_buf .. str
	end
	function fd:expect(len, cb)
		table.insert(self.rd_expect, {len, "", cb})
	end
	return fd
end

function mainloop.add_fd(fd, error_cb)
	local fd = new_fd(fd, error_cb)
	table.insert(fds, fd)
	mainloop_fds_change = true
	return fd
end

function mainloop.run()
	mainloop_must_exit = false
	sel_fds = {}
	while not mainloop_must_exit do
		local deadlock = true
		for i, fd in pairs(fds) do
			if mainloop_fds_change then
				sel_fds[i] = {fd.fd}
			end
			local flags = sysdef.SEL_ERROR
			if fd.rd_expect[1] then
				flags = flags | sysdef.SEL_READ
				deadlock = false
			end
			if #fd.wr_buf > 0 then
				flags = flags | sysdef.SEL_WRITE
				deadlock = false
			end
			sel_fds[i][2] = flags
		end
		mainloop_fds_change = false
		assert(not deadlock, "Mainloop does nothing!")

		local res = sys.select(sel_fds, -1)
		assert(res, "select() call failed")
		for i, fd in pairs(fds) do
			local flags = sel_fds[i][3]
			if flags & sysdef.SEL_ERROR ~= 0 then
				fd.error_cb(fd)
			else
				if flags & sysdef.SEL_READ ~= 0 then
					local reader = fd.rd_expect[1]
					local tmp = sys.read(fd.fd, 0, reader[1] - #reader[2])
					reader[2] = reader[2] .. tmp
					if #reader[2] == reader[1] then
						reader[3](reader[2])
						table.remove(fd.rd_expect, 1)
					end
				end
				if flags & sysdef.SEL_WRITE ~= 0 then
					local r = sys.write(fd.fd, 0, fd.wr_buf)
					fd.wr_buf = fd.wr_buf:sub(r+1)
				end
			end
		end
	end
end

function mainloop.exit()
	mainloop_must_exit = true
end

return mainloop
