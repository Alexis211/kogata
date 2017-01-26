print("Hello, world! (printed)")

local t = {}

function t.decomp(x)
	local ret = {}
	for i = 2, x do
		while x % i == 0 do
			table.insert(ret, i)
			x = x // i
		end
	end
	return ret
end

function t.pretty(n)
	for i = 1, n do
		local d = t.decomp(i)
		for _, v in pairs(d) do
			io.stdout:write(('-'):rep(v) .. ' ')
		end
		io.stdout:write('\n')
	end
end

function t.guess(n)
	local a = math.random(n)
	while true do
		io.stdout:write("Guess? ")
		local x = tonumber(io.stdin:read())
		if x < a then
			print("Too small!")
		elseif x > a then
			print("Too big!")
		else
			print("Bingo!")
			break
		end
	end
end

return t
