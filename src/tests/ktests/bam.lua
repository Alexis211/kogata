local obj_nokmain = {}
for name in TableWalk(kernel_obj) do
	if not name:find('kmain') then
		table.insert(obj_nokmain, name)
	end
end

for _, name in pairs({
	"breakpoint",
	"btree1", "btree2",
	"cmdline",
	"hashtbl1", "hashtbl2",
	"kmalloc",
	"region1"
}) do
	local map = "build/tests/ktest_" .. name .. ".map"

	local test_config = TableDeepCopy(common_settings)
	test_config.cc.includes:Add("src/kernel/include",
								"src/tests/ktests/"..name)
	test_config.cc.flags:Add('-DBUILD_KERNEL_TEST')
	test_config.link.flags:Add("-T src/kernel/linker.ld",
							   "-Xlinker -Map=" .. map)
	test_config.cc.extension = ".ktest_" .. name .. ".o"

	local kmain = Compile(test_config, "src/kernel/core/kmain.c")
	local bin = Link(test_config, "tests/ktest_"..name,
					 {obj_nokmain, kmain, common_libkogata, common_libc, common_libalgo})

	local out = "build/tests/ktest_"..name..".log"
	AddJob(out, "ktest " .. name, "./src/tests/ktests/run_qemu_test.sh " .. bin .. " " .. out .. " " .. map)
	AddDependency(out, bin)
	table.insert(tests, out)
end

