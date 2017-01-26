print "Lua helpers for Kogata v1"

do
	local old_tostring = tostring
	function tostring(x)
		if type(x) == "table" then
			if next(x) == nil then
				return '{}'
			end
			local q = '{\n  '
			for k, v in pairs(x) do
				if q:len() > 4 then
					q = q .. ',\n  '
				end
				q = q .. k .. ': ' .. tostring(v):gsub('\n', '\n  ')
			end
			return q .. '\n}'
		else
			return old_tostring(x)
		end
	end
end
