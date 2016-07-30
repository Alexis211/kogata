return function(s, kernel, lib)
	local tests = {}

	for _, name in pairs({
		"chan1", "chan2",
		"fs1", "fs2",
		"malloc",
		"subfs"
	}) do
		local config = TableDeepCopy(s.user_settings)
		config.link.flags:Add('-T src/lib/linker.ld')

		local obj = Compile(config, 'src/tests/utests/' .. name .. '/test.c')
		local bin = Link(config, 'tests/utest_' .. name, {obj, lib.libkogata})

		local out = bin .. ".log"
		AddJob(out, "utest " .. name, "./src/tests/utests/run_qemu_test.sh " .. bin .. " " .. out .. " " .. kernel)
		AddDependency(out, bin)
		AddDependency(out, kernel)
		table.insert(tests, out)
	end

	return tests
end
