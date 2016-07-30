return function(s, lib)
	local function bin_settings(name)
		local s = TableDeepCopy(s.user_settings)
		s.link.flags:Add("-T src/lib/linker.ld")
		if name == 'lua' or name == 'luac' then
			s.cc.includes:Add("src/lib/lua")
		end
		return s
	end

	local function bin_exe(name, deps)
		local s = bin_settings(name)

		local src = Collect('src/bin/' .. name .. '/*.c')
		local obj = Compile(s, src)

		return Link(s, 'bin/' .. name .. ".bin", {obj, deps})
	end

	return {
		bin_exe('lua', {lib.liblua}),
		-- bin_exe('luac', {liblua}),
	}
end
