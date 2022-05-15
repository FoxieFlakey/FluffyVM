-- Abusing lua as assembly
local strHello = const("Hello World!")
local strPrint = const("print")
local strPogString = const("Pogger fox this working")

local ENV_TABLE = 0xFFFE
local NIL = 0xFFFF
local COND_NONE = 0x00

-- Executable
get_constant(COND_NONE, 0x0001, strPrint) -- Get "print" constant
get_constant(COND_NONE, 0x0002, strHello) -- Get "Hello World!" constant

-- Get "print" function from env table
table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)

-- Push "Hello World!" string to stack
stack_push(COND_NONE, 0x0002)

-- Call the "print" function with one argument
-- in stack and zero return
call(COND_NONE, 0x0001, 0, 0, 0, 2)

-- Return zero values
ret(COND_NONE, 0, 0)

start_prototype()
nop()
mov(0, 0, 0)
table_set(0, 0, 0, 0)

start_prototype()
stack_push(0, 0)
stack_pop(0, 0)
stack_get_top(0, 0)
end_prototype()

end_prototype()
