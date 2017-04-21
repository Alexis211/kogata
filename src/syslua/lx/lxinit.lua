print "Lua eXtended helpers for Kogata v1"

do
	local old_tostring = tostring
	function tostring(x)
    local seen = {}
    function aux(x)
      if type(x) == "table" then
        if next(x) == nil then
          return '{}'
        end

        if seen[x] then
          return '...'
        end
        seen[x] = true

        local q = '{\n  '
        for k, v in pairs(x) do
          if q:len() > 4 then
            q = q .. ',\n  '
          end
          q = q .. k .. ': ' .. aux(v):gsub('\n', '\n  ')
        end
        return q .. '\n}'
      else
        return old_tostring(x)
      end
    end
    return aux(x)
	end
end


function string.split(str, sep)
   local sep, fields = sep or ":", {}
   local pattern = string.format("([^%s]*)", sep)
   str:gsub(pattern, function(c) fields[#fields+1] = c end)
   return fields
end

function hexdump(str)
	for i = 1, #str, 16 do
		local b = {string.byte(str, i, math.min(i+15, #str))}
		local s = ""
		for j = 1, 16 do
			if b[j] then
				s = s .. string.format("%02x ", b[j])
			else
				s = s .. '   '
			end
		end
		s = s .. '|  '
		for j = 1, #b do
			ss = string.char(b[j])
			if b[j] >= 32 and b[j] < 128 then
				s = s .. string.char(b[j])
			else
				s = s .. '.'
			end
		end
		print(s)
	end
end
