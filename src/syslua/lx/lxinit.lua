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
