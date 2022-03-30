from dataclasses import dataclass
from lib import serializationPackage as sp
import typing
import time


@dataclass
class VMRuntimeReadOnlyData:
    """Class for keeping track of an item in inventory."""
    stringmap: typing.Dict[int, str]
    contextmap: typing.Dict[int, sp.Context]
    initial_context_ptr: int

@dataclass
class CallStackSingleElement:
    funcPointer: int
    ip: int
    slot_offset: int
    upvalues: typing.List[sp.Upvalue]

@dataclass
class VMRuntimeWriteOnlyData:
    callstack: typing.List[CallStackSingleElement]
    global_data: typing.Dict

@dataclass
class RuntimeClosure:
    function_ptr: int
    upvalues: typing.List[sp.Upvalue]

@dataclass 
class LoxClass:
    name: str
    methods: typing.Dict[str, RuntimeClosure]

@dataclass
class LoxInstance:
    klass: LoxClass
    fields: typing.Dict[str, typing.Any]

@dataclass
class RuntimeBoundMethod:
    lox_instance: LoxInstance
    closure: RuntimeClosure

def load_vmdata(path: str) -> sp.VMData():
    vmdata = sp.VMData()
    f = open(path, "rb")
    vmdata.parse(f.read())
    f.close()
    return vmdata

def generate_vm_data(path: str):
    vmdata = load_vmdata(path)

    runtime_string_map = build_string_map(vmdata.strings_at_addresses)
    runtime_instruction_context_map, initial_context_ptr = build_instruction_map_and_initial_context(vmdata.contexts)
    vm_runtime_read_only_main = VMRuntimeReadOnlyData(runtime_string_map, runtime_instruction_context_map, initial_context_ptr)
    vmRuntimeCallstack = []
    data_stack = [vm_runtime_read_only_main.contextmap[initial_context_ptr]]
    vm_runtime_write_only_main = VMRuntimeWriteOnlyData(vmRuntimeCallstack, {})
    define_native("clock", clock_native, data_stack, vm_runtime_write_only_main.global_data)
    
    return vm_runtime_read_only_main, vm_runtime_write_only_main, data_stack, initial_context_ptr

def clock_native(arg_count: int, args):
    return time.process_time()

def define_native(name: str, function_body, data_stack: typing.List, global_data):
    data_stack.append(name)
    data_stack.append(function_body)
    global_data[name] = function_body
    data_stack.pop()
    data_stack.pop()

def build_string_map(string_data):
    string_map = {}
    for string_datum in string_data:
        string_map[string_data[string_datum].address] = string_datum
    
    return string_map

def build_instruction_map_and_initial_context(contexts: typing.List[sp.Context]):
    context_map = {}
    initial_context_ptr = -1
    for context in contexts:
        context_ptr = context.function_address
        #sort dict here:
        context.instruction_vals = dict(sorted(context.instruction_vals.items(), key=lambda item: item[0]))
        context_map[context_ptr] = context
        if context.context_name == "":
            initial_context_ptr = context_ptr
    return context_map, initial_context_ptr

def new_closure(context: sp.Context):
    num_of_upvalues = context.upvalue_count
    return RuntimeClosure(context.function_address, [])