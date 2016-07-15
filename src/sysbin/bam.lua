local function sysbin_settings(name)
	local s = TableDeepCopy(user_settings)
	s.link.flags:Add("-T src/sysbin/linker.ld",
					 "-Xlinker -Map=build/sysbin/" .. name .. ".map")
	return s
end

local function sysbin_exe(name, deps)
	local s = sysbin_settings(name)

	local src = Collect('src/sysbin/' .. name .. '/*.c')
	local obj = Compile(s, src)

	return Link(s, 'sysbin/' .. name .. ".bin", {obj, deps})
end

sysbin = {
	sysbin_exe('init', {libkogata}),
	sysbin_exe('giosrv', {libkogata}),
	sysbin_exe('login', {libkogata}),
	sysbin_exe('terminal', {libkogata}),
	sysbin_exe('shell', {libkogata}),
}
