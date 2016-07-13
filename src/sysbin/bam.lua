local function sysbin_settings(name)
	local s = TableDeepCopy(user_settings)
	s.link.flags:Add("-T src/sysbin/linker.ld",
					 "-Xlinker -Map=build/" .. name .. ".map")
	return s
end

local function sysbin_exe(name)
	local s = sysbin_settings(name)

	local src = Collect('src/sysbin/' .. name .. '/*.c')
	local obj = Compile(s, src)

	return Link(s, name .. ".bin", {obj, libkogata})
end

sysbin = {
	sysbin_exe('init'),
	sysbin_exe('giosrv'),
	sysbin_exe('login'),
	sysbin_exe('terminal'),
	sysbin_exe('shell'),
}
