-- Abusing lua as assembly
local strHello = str_const("Hello World!")
local strPrint = str_const("print")

local ENV_TABLE = 0xFFFE
local NIL = 0xFFFF

-- Executable
get_constant(0x0001, 0) -- Get "Hello World!" constant
get_constant(0x0002, 1) -- Get "print" constant

-- Get "print" function from env table
table_get(0x0001, ENV_TABLE, 0x0001)

-- Push "Hello World!" string to stack
stack_push(0x0002)

-- Call the "print" function with one argument
-- in stack and zero return
call(0x0001, 0, 0, 0, 2)

-- Return zero values
ret(0, 0)
