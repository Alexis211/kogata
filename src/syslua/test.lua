print("Hello, world! (printed)")


return function(n)
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

