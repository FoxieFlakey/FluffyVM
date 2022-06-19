local function fact(n)
	if n == 0 then
		return 1
	else
		return n * fact(n - 1)
	end
end

print(fact(2))
print(fact(4))
print(fact(16))
