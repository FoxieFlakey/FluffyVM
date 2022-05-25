-- Abusing lua as assembly
local strHello = const("Hello World!")
local strPrint = const("print")
local strReturnString = const("return_string")
local strPogString = const("Pogger fox this working")
local strAProtoString = const("a_proto")

local strFromAPrototype = const("This message is from inside a prototype")
local strFromAPrototypeInsideAnotherPrototype = const("This message is from a prototype that is inside another prototype")
local strFromAPrototypeSettedInTheEnv = const("This message is from a prototype setted in env table")

local ENV_TABLE = 0xFFFE
local NIL = 0xFFFF
local COND_NONE = 0x00

get_constant(COND_NONE, 0x0001, strAProtoString)
load_prototype(COND_NONE, 0x0002, 0)
table_set(COND_NONE, ENV_TABLE, 0x0001, 0x0002)
stack_push(COND_NONE, 0x0002)

stack_push(COND_NONE, ENV_TABLE)

get_constant(COND_NONE, 0x0001, strHello)
stack_push(COND_NONE, 0x0001)

get_constant(COND_NONE, 0x0001, strPogString)
stack_push(COND_NONE, 0x0001)

-- Call the "return_string" function
get_constant(COND_NONE, 0x0001, strReturnString)
table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)
call(COND_NONE, 0x0001, 1, 0, 0)

-- Call the "print" function
get_constant(COND_NONE, 0x0001, strPrint)
table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)
call(COND_NONE, 0x0001, 0, 0, 1)

stack_pop(COND_NONE, NIL)

-- Print all string except the last string
-- returned by `return_string`
call(COND_NONE, 0x0001, 0, 0, 1)

load_prototype(COND_NONE, 0x0001, 1)
call(COND_NONE, 0x0001, 0, 0, 0)

ret(COND_NONE, 0, 0)

--
-- Prototypes under here
--

start_prototype()
  get_constant(COND_NONE, 0x0001, strFromAPrototypeSettedInTheEnv)
  stack_push(COND_NONE, 0x0001)

  get_constant(COND_NONE, 0x0001, strPrint)
  table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)

  call(COND_NONE, 0x0001, 0, 0, 2)

  ret(COND_NONE, 0, 0)
end_prototype()

start_prototype()
  get_constant(COND_NONE, 0x0001, strFromAPrototype)
  stack_push(COND_NONE, 0x0001)

  get_constant(COND_NONE, 0x0001, strPrint)
  table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)

  call(COND_NONE, 0x0001, 0, 0, 2)
  
  load_prototype(COND_NONE, 0x0001, 0)
  call(COND_NONE, 0x0001, 0, 0, 0)
  ret(COND_NONE, 0, 0)

  --
  -- Prototypes under here
  --
  start_prototype()
    get_constant(COND_NONE, 0x0001, strFromAPrototypeInsideAnotherPrototype)
    stack_push(COND_NONE, 0x0001)

    get_constant(COND_NONE, 0x0001, strPrint)
    table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)

    call(COND_NONE, 0x0001, 0, 0, 2)
    
    get_constant(COND_NONE, 0x0001, strAProtoString)
    table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)
    call(COND_NONE, 0x0001, 0, 0, 0)

    ret(COND_NONE, 0, 0)
  end_prototype()
end_prototype()



