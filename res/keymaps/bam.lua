keymaps = {}

for _, file in pairs(Collect('res/keymaps/*.c')) do
	local obj = Compile(host_settings, file)
	local tgt = BuildOutput(host_settings, PathBase(file) .. '_tmp')
	local bin = Link(host_settings, tgt, obj)

	local out = BuildOutput(host_settings, PathBase(file)) .. '.km'
	AddJob(out, "call font " .. bin, "./" .. bin .. " > " .. out)
	AddDependency(out, bin)
	table.insert(keymaps, out)
end
