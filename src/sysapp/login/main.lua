local sys = require 'lx.sys'
local sysdef = require 'lx.sysdef'

local mainloop = require 'lx.mainloop'
local gip = require 'lx.gip'
local draw = require 'lx.draw'


local io = gip.new(sysdef.STD_FD_GIP)

function io:damage(reg)
  if self.features & gipdef.GIPF_DAMAGE_NOTIF ~= 0 then
    self:send_buffer_damage(draw.region(0, 0, 256, 256))
  end
end

function io:on_initiate(arg)
  self.flags = arg

  self:async_enumerate_modes(function(modes)
    sys.dbg_print("Got mode list:\n")
    for i, v in pairs(modes) do
      sys.dbg_print(string.format("%d: %dx%d %d\n", i, v.width, v.height, v.bpp))
    end

    for i, v in pairs(modes) do
      if v.width == 800 and v.height == 600 and v.bpp == 24 then
        sys.dbg_print(string.format("Selecting mode %d\n", i))
        io:send_set_mode(i)
        return
      end
    end
  end)
end

function io:on_buffer_info(arg, tok, geom)
  self.surface = draw.surface_from_fd(sys.use_token(tok))
  self.geom = geom

  for x = 0, 255 do
    for y = 0, 255 do
      self.surface:put(x, y, draw.rgb(x, y, 128))
    end
  end

  self:damage(draw.region(0, 0, 256, 256))
end

function io:async_enumerate_modes(callback)
  local modelist = {}
  local function gotmode(cmd, arg)
    if arg == nil then
      callback(modelist)
    else
      modelist[#modelist + 1] = arg
      io.on_reply[io:send_query_mode(#modelist)] = gotmode
    end
  end
  io.on_reply[io:send_query_mode(#modelist)] = gotmode
end

mainloop.add(io)

io:send_reset()
mainloop.run()



