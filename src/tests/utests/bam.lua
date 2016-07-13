for _, name in pairs({
	"chan1", "chan2",
	"fs1", "fs2",
	"malloc",
	"subfs"
}) do
	local map = "build/tests/utest_" .. name .. ".map"

	local config = TableDeepCopy(user_settings)
	config.link.flags:Add( '-Xlinker -Map=' .. map,
						  '-T src/sysbin/linker.ld')

	local obj = Compile(config, 'src/tests/utests/' .. name .. '/test.c')
	local bin = Link(config, 'tests/utest_' .. name, {obj, libkogata})

	local out = "build/tests/utest_"..name..".log"
	AddJob(out, "utest " .. name, "./src/tests/utests/run_qemu_test.sh " .. bin .. " " .. out .. " " .. map)
	AddDependency(out, bin)
	AddDependency(out, kernel)
	table.insert(tests, out)
end
