local function bin_settings(name)
	local s = TableDeepCopy(user_settings)
	s.link.flags:Add("-T src/sysbin/linker.ld",
					 "-Xlinker -Map=build/bin/" .. name .. ".map")
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

bin = {
	-- bin_exe('lua', {liblua}),
	-- bin_exe('luac', {liblua}),
}
