keymaps = {}

for _, file in pairs(Collect('res/keymaps/*.c')) do
	local obj = Compile(host_settings, file)
	local tgt = PathJoin('build', PathBase(file) .. '_tmp')
	local bin = Link(host_settings, tgt, obj)

	local out = PathJoin('build', PathBase(file) .. '.km')
	AddJob(out, "call font " .. bin, "./" .. bin .. " > " .. out)
	AddDependency(out, bin)
	table.insert(keymaps, out)
end
