local sys = require 'lx.sys'
local sysdef = require 'lx.sysdef'

local _cwd = 'root:/'

function pathcat(path1, path2)
  assert(path1, "invalid argument")
  if not path2 then
    return path1
  end

  function explode_path(path)
    local _, _, dr, p = string.find(path, '^(%w+):(.*)$')
    if not dr or not p then
      dr, p = nil, path
    end
    local pp = string.split(p, '/')
    if #pp > 1 and pp[#pp] == '' then
      table.remove(pp)
    end
    return dr, pp
  end

  function implode_path(dr, p)
    assert(p[1] == '', 'bad first path component')
    return dr .. ':' .. table.concat(p, '/')
  end

  local dr2, p2 = explode_path(path2)
  if dr2 then
    return implode_path(dr2, p2)
  else
    local dr, p1 = explode_path(path1)
    assert(p1[1] == '', 'bad path1!')
    local p = p1
    if p2[1] == '' then
      p = {}
    end
    for _, v in pairs(p2) do
      if v == '..' then
        table.remove(p)
      elseif v ~= '.' then
        table.insert(p, v)
      end
    end
    return implode_path(dr, p)
  end
end

function cd(path)
  -- TODO path simplification etc
  local newcwd = pathcat(_cwd, path)
  local s = sys.stat(newcwd)
  if not s then
    print("not found: " .. newcwd)
  elseif s.type & sysdef.FT_DIR == 0 then
    print("not a directory: " .. newcwd)
  else
    _cwd = newcwd
    print(_cwd)
  end
end

function cwd()
  return _cwd
end

function ls(path)
  path = pathcat(_cwd, path)

  local fd = sys.open(path, sysdef.FM_READDIR)
  if not fd then
    print("Could not open " .. path)
  else
    local i = 0
    while true do
      x = sys.readdir(fd, i)
      if not x then break end
      if x.type & sysdef.FT_DIR ~= 0 then
        x.name = x.name .. '/'
      end
      print(x.type, x.access, x.size, x.name)
      i = i + 1
    end
    sys.close(fd)
  end
end

