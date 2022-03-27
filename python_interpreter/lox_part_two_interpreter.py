from recordclass import recordclass, RecordClass
from dis import dis
import sys
import typing
import operator
import betterproto
from lib import serializationPackage as sp

VMRuntimeReadOnlyData = recordclass("VMRuntimeReadOnlyData", 'stringmap contextmap initial_context_ptr')
CallStackSingleElement = recordclass("CallStackDataModel", 'funcPointer ip slot_offset')
VMRuntimeWriteOnlyData = recordclass("VMRuntimeWriteOnlyData", 'callstack global_data')

CALL_STACK_MAX = 100

def is_falsey(value):
    return value is None or value == False

def read_byte(vm_read_only: VMRuntimeReadOnlyData, call_stack: CallStackSingleElement):
    context: sp.Context = vm_read_only.contextmap[call_stack.funcPointer]
    instruction_counter = call_stack.ip
    instruction_value = None
    if (betterproto.which_one_of(context.instruction_vals[instruction_counter], "InstructionTypes")[0] == 'opcode'):
        instruction_value = context.instruction_vals[instruction_counter].opcode
    elif (betterproto.which_one_of(context.instruction_vals[instruction_counter], "InstructionTypes")[0] == 'address_or_constant'):
        instruction_value = context.instruction_vals[instruction_counter].address_or_constant
    else:
        print("Error: incorrect type found!")
    return instruction_value, instruction_counter + 1 

def read_constant(vm_read_only: VMRuntimeReadOnlyData, call_stack: CallStackSingleElement):
    byte_num, instruction_counter = read_byte(vm_read_only, call_stack)
    context: sp.Context =  vm_read_only.contextmap[call_stack.funcPointer]
    current_value = context.constant_vals[byte_num]
    # breakpoint()
    if (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'string_address'):
        return (vm_read_only.stringmap[current_value.string_address], instruction_counter)
    elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'function_address'):
        return (vm_read_only.contextmap[current_value.function_address], instruction_counter)
    elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'num_val'):
        return (current_value.num_val, instruction_counter)
    elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'bool_val'):
        return (current_value.bool_val, instruction_counter)

def read_short(vm_read_only: VMRuntimeReadOnlyData, call_stack: CallStackSingleElement):
    new_instruction_counter = call_stack.ip + 2
    context: sp.Context =  vm_read_only.contextmap[call_stack.funcPointer]
    # instruction_value = vmdata.instructions[value]

    second_last_instruction = context.instruction_vals[new_instruction_counter - 2].address_or_constant if \
        betterproto.which_one_of(context.instruction_vals[new_instruction_counter - 2], "InstructionTypes")[0] == 'address_or_constant' \
        else context.instruction_vals[new_instruction_counter - 2].opcode

    first_value = second_last_instruction << 8 

    last_instruction = context.instruction_vals[new_instruction_counter - 1].address_or_constant if \
        betterproto.which_one_of(context.instruction_vals[new_instruction_counter - 1], "InstructionTypes")[0] == 'address_or_constant' \
        else context.instruction_vals[new_instruction_counter - 1].opcode

    second_value = first_value | last_instruction
    # first_value = vm_read_only.instructions[new_instruction_counter - 2] << 8
    # second_value = first_value | vmdata.instructions[new_instruction_counter - 1]
    return second_value, new_instruction_counter

# def read_byte(vmdata:sp.VMData, current: int):
#     # breakpoint()
#     return (vmdata.instructions[current], current + 1)


# def read_constant(vmdata:sp.VMData, current: int):
#     byte_num, instruction_counter = read_byte(vmdata, current)
#     current_value =  vmdata.constant_vals[byte_num]
#     # breakpoint()
#     if (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'object_address'):
#         return (vmdata.string_map[current_value.object_address], instruction_counter)
#     elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'num_val'):
#         return (current_value.num_val, instruction_counter)
#     elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'bool_val'):
#         return (current_value.bool_val, instruction_counter)

def read_string(vmdata:sp.VMData, current: int):
    return read_constant(vmdata, current)

# def read_short(vmdata:sp.VMData, current: int):
#     new_instruction_counter = current + 2
#     # instruction_value = vmdata.instructions[value]
#     first_value = vmdata.instructions[new_instruction_counter - 2] << 8
#     second_value = first_value | vmdata.instructions[new_instruction_counter - 1]
#     return second_value, new_instruction_counter

def runtimeError(instruction_text: str, vmRuntimeReadOnlyData: VMRuntimeReadOnlyData, 
        call_stack: typing.List[CallStackSingleElement]):
    print(f'[error: {instruction_text}] in script')
    for i in reversed(range(len(call_stack))):
        context_pointer = call_stack[i].funcPointer
        context_function_name = vmRuntimeReadOnlyData.contextmap[context_pointer].context_name 
        if (context_function_name == None):
            print("script\n")
        else:
            print(f'{context_function_name}()')

def run_file(path: str):
    vmdata = load_vmdata(path)

    runtime_string_map = build_string_map(vmdata.strings_at_addresses)
    runtime_instruction_context_map, initial_context_ptr = build_instruction_map_and_initial_context(vmdata.contexts)
    vm_runtime_read_only_main = VMRuntimeReadOnlyData(runtime_string_map, runtime_instruction_context_map, initial_context_ptr)
    vmRuntimeCallstack = []
    data_stack = [vm_runtime_read_only_main.contextmap[initial_context_ptr]]
    call(vm_runtime_read_only_main, vm_runtime_read_only_main.contextmap[initial_context_ptr], 0, vmRuntimeCallstack, data_stack)
    vm_runtime_write_only_main = VMRuntimeWriteOnlyData(vmRuntimeCallstack, {})

    run(vm_runtime_read_only_main, vm_runtime_write_only_main, data_stack)

def load_vmdata(path: str) -> sp.VMData():
    vmdata = sp.VMData()
    f = open(path, "rb")
    vmdata.parse(f.read())
    f.close()
    return vmdata


def binaryOp(vm_runtime_read_only_main, vmRuntimeCallstack, current_stack, passed_func, the_type):
    while True:
        if not type(current_stack[-1]) == float or not type(current_stack[-2]) == float:
            runtimeError("Operands must be numbers.", vm_runtime_read_only_main, vmRuntimeCallstack)
            return False
        arg_b = current_stack.pop()
        arg_a = current_stack.pop()
        if (the_type == bool):
            current_stack.append(passed_func(arg_a, arg_b))
        elif (the_type == float):
            current_stack.append(passed_func(arg_a, arg_b))
        break
    return True

def concatenate(str1: str, str2: str):
    total_str = str1 + str2
    return total_str

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

def get_first_instruction(vmReadOnlyData: VMRuntimeReadOnlyData, function_ptr: int):
    curr_context = vmReadOnlyData.contextmap[function_ptr]
    return curr_context.first_instruction_address


def run(vm_runtime_read_only_main, vm_runtime_write_only_main, data_stack):
    vmRuntimeCallstack = vm_runtime_write_only_main.callstack

    while True:
        instruction_value, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
        print("Executing instruction value: " + str(sp.ContextOpcode(instruction_value)))
        if instruction_value == sp.ContextOpcode.OP_CONSTANT:
            constant, vmRuntimeCallstack[-1].ip = read_constant(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            data_stack.append(constant)
        elif instruction_value == sp.ContextOpcode.OP_PRINT:
            print(data_stack.pop())
        elif instruction_value == sp.ContextOpcode.OP_RETURN:
            result = data_stack.pop()
            truncated_end_value = vmRuntimeCallstack[-1].slot_offset
            vmRuntimeCallstack.pop()
            if len(vmRuntimeCallstack) == 0:
                data_stack.pop()
                return True
            data_stack = data_stack[:truncated_end_value]
            data_stack.append(result)
        
        elif instruction_value == sp.ContextOpcode.OP_NIL:
            data_stack.append(None)
        elif instruction_value == sp.ContextOpcode.OP_TRUE:
            data_stack.append(True)
        elif instruction_value == sp.ContextOpcode.OP_FALSE:
            data_stack.append(False)   
        elif instruction_value == sp.ContextOpcode.OP_GET_GLOBAL:
            name, vmRuntimeCallstack[-1].ip = read_constant(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            if (name not in vm_runtime_write_only_main.global_data):
                runtimeError(f'Undefined variable \'{name}\'.', vm_runtime_read_only_main, vmRuntimeCallstack)
                return False
            data_stack.append(vm_runtime_write_only_main.global_data[name])
        elif instruction_value == sp.ContextOpcode.OP_DEFINE_GLOBAL:
            name, vmRuntimeCallstack[-1].ip = read_constant(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            vm_runtime_write_only_main.global_data[name] = data_stack[-1]
            data_stack.pop()
        elif instruction_value == sp.ContextOpcode.OP_SET_GLOBAL:
            name, vmRuntimeCallstack[-1].ip = read_constant(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            if (name not in vm_runtime_write_only_main.global_data):
                runtimeError(f'Undefined variable \'{name}\'.', vm_runtime_read_only_main, vmRuntimeCallstack)
                return False
            else:
                value = data_stack[-1]
                vm_runtime_write_only_main.global_data[name] = value
        elif instruction_value == sp.ContextOpcode.OP_GET_LOCAL: 
            slot, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            data_stack.append(data_stack[vmRuntimeCallstack[-1].slot_offset + slot])
        elif instruction_value == sp.ContextOpcode.OP_SET_LOCAL: 
            slot, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            data_stack[vmRuntimeCallstack[-1].slot_offset + slot] = data_stack[-1]
        elif instruction_value == sp.ContextOpcode.OP_SUBTRACT:
            if (binaryOp(vm_runtime_read_only_main, vmRuntimeCallstack, data_stack, operator.sub, float) == False):
                return False
        elif instruction_value == sp.ContextOpcode.OP_MULTIPLY:
            if (binaryOp(vm_runtime_read_only_main, vmRuntimeCallstack, data_stack, operator.mul, float) == False):
                return False
        elif instruction_value == sp.ContextOpcode.OP_DIVIDE:
            if (binaryOp(vm_runtime_read_only_main, vmRuntimeCallstack, data_stack, operator.truediv, float) == False):
                return False
        elif instruction_value == sp.ContextOpcode.OP_NOT:
            data_stack.append(is_falsey(data_stack.pop()))
        elif instruction_value == sp.ContextOpcode.OP_NEGATE:
            if type(data_stack[-1]) != float:
                runtimeError("Operand must be a number.", vm_runtime_read_only_main, vmRuntimeCallstack)
                return False
            data_stack.append(-(data_stack.pop()))
        elif instruction_value == sp.ContextOpcode.OP_POP:
            data_stack.pop()
        elif instruction_value == sp.ContextOpcode.OP_EQUAL:
            b = data_stack.pop()
            a = data_stack.pop()
            data_stack.append(a == b)
        elif instruction_value == sp.ContextOpcode.OP_GREATER:
            if binaryOp(vm_runtime_read_only_main, vmRuntimeCallstack, data_stack, operator.gt, bool) == False:
                return False

        elif instruction_value == sp.ContextOpcode.OP_LESS:
            if binaryOp(vm_runtime_read_only_main, vmRuntimeCallstack, data_stack, operator.lt, bool) == False:
                return False

        elif instruction_value == sp.ContextOpcode.OP_ADD:
            # breakpoint()
            if type(data_stack[-1]) == str and type(data_stack[-2]) == str:
                # breakpoint()
                val_2 = data_stack.pop()
                val_1 = data_stack.pop()
                data_stack.append(concatenate(val_1, val_2))
            elif type(data_stack[-1]) == float and type(data_stack[-2]) == float:
                data_stack.append(data_stack.pop() + data_stack.pop())
            else:
                runtimeError("Operands must be two numbers or two strings.", vm_runtime_read_only_main, vmRuntimeCallstack)
                return False    
        elif instruction_value == sp.ContextOpcode.OP_LOOP:
            offset, vmRuntimeCallstack[-1].ip = read_short(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            vmRuntimeCallstack[-1].ip -= offset
        elif instruction_value == sp.ContextOpcode.OP_JUMP:
            offset, vmRuntimeCallstack[-1].ip = read_short(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            vmRuntimeCallstack[-1].ip += offset
        elif instruction_value == sp.ContextOpcode.OP_JUMP_IF_FALSE:
            offset, vmRuntimeCallstack[-1].ip = read_short(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            if is_falsey(data_stack[-1]):
                vmRuntimeCallstack[-1].ip += offset
        elif instruction_value == sp.ContextOpcode.OP_CALL:
            arg_count, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            if (not call_value(vm_runtime_read_only_main, vmRuntimeCallstack, data_stack, 
                    data_stack[-(arg_count + 1)], arg_count)):
                return False

def call_value(vm_runtime_read_only_main: VMRuntimeReadOnlyData, call_stack: typing.List[CallStackSingleElement], 
        data_stack: typing.List, callee_function_context: sp.Context, arg_count: int) -> bool:
    new_function_address = callee_function_context.function_address
    if (new_function_address not in vm_runtime_read_only_main.contextmap):
        print("Can only call functions and classes")
    else:
        #Todo: Fix
        return call(vm_runtime_read_only_main, callee_function_context, arg_count, call_stack, data_stack)

def call(vm_runtime_read_only_main: VMRuntimeReadOnlyData, context: sp.Context, arg_count: int, call_stack: typing.List[CallStackSingleElement], data_stack: typing.List) -> bool:
    if (arg_count != context.arity):
        # runtimeError(5, call_stack)
        runtimeError(f'Expected {context.arity} arguments but got {arg_count}', vm_runtime_read_only_main, call_stack)
        return False
    if (len(call_stack) > CALL_STACK_MAX):
        # print("Stack overflow")
        runtimeError("Stack overflow", vm_runtime_read_only_main, call_stack)
        return False
    call_stack.append(CallStackSingleElement(context.function_address, context.first_instruction_address, len(data_stack) -(arg_count + 1)))
    return True

def main():
    if len(sys.argv) > 2 or len(sys.argv) < 2:
        print("Usage: plox [script]")
    elif len(sys.argv) == 2:
        run_file(sys.argv[1])

if __name__ == "__main__":
    main()
