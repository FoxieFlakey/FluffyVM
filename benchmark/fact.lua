local start = os.clock()

for i=1,1000000 do
  local product = 1
  for j=1,16 do
    product = product * j
  end
end

print("Time used: "..((os.clock() - start) * 1000).." ms")
