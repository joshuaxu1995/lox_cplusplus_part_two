from recordclass import recordclass, RecordClass
from dis import dis
import sys
import types
import typing
import operator
import betterproto
from lib import serializationPackage as sp
from vm_data_generator import VMRuntimeReadOnlyData, VMRuntimeWriteOnlyData, CallStackSingleElement, RuntimeClosure, generate_vm_data, new_closure

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

    second_last_instruction = context.instruction_vals[new_instruction_counter - 2].address_or_constant if \
        betterproto.which_one_of(context.instruction_vals[new_instruction_counter - 2], "InstructionTypes")[0] == 'address_or_constant' \
        else context.instruction_vals[new_instruction_counter - 2].opcode

    first_value = second_last_instruction << 8 

    last_instruction = context.instruction_vals[new_instruction_counter - 1].address_or_constant if \
        betterproto.which_one_of(context.instruction_vals[new_instruction_counter - 1], "InstructionTypes")[0] == 'address_or_constant' \
        else context.instruction_vals[new_instruction_counter - 1].opcode

    second_value = first_value | last_instruction
    return second_value, new_instruction_counter

def read_string(vmdata:sp.VMData, current: int):
    return read_constant(vmdata, current)

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


def run(vm_runtime_read_only_main: VMRuntimeReadOnlyData, vm_runtime_write_only_main: VMRuntimeWriteOnlyData, data_stack: typing.List):
    vmRuntimeCallstack = vm_runtime_write_only_main.callstack

    while True:
        instruction_value, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
        # print("Executing instruction value: " + str(sp.ContextOpcode(instruction_value)))
        if instruction_value == sp.ContextOpcode.OP_CONSTANT:
            constant, vmRuntimeCallstack[-1].ip = read_constant(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            data_stack.append(constant)
        elif instruction_value == sp.ContextOpcode.OP_PRINT:
            print(data_stack.pop())
        elif instruction_value == sp.ContextOpcode.OP_CLOSURE:
            temp_context, vmRuntimeCallstack[-1].ip = read_constant(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            closure = new_closure(temp_context)
            data_stack.append(closure)
            for i in range(temp_context.upvalue_count): #TODO: Potential for a bug to live here
                is_local, vmRuntimeCallstack[-1].ip  = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
                index, vmRuntimeCallstack[-1].ip  = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
                if (is_local > 0):
                    closure.upvalues.append(data_stack[vmRuntimeCallstack[-1].slot_offset + index])
                else:
                    closure.upvalues.append(vmRuntimeCallstack[-1].upvalues[index])
                    
        elif instruction_value == sp.ContextOpcode.OP_GET_UPVALUE:
            slot, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            data_stack.append(vmRuntimeCallstack[-1].upvalues[slot])

        elif instruction_value == sp.ContextOpcode.OP_SET_UPVALUE:
            slot, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])
            vmRuntimeCallstack[-1].upvalues[slot] = data_stack[-1] #TODO: Fix
            

        elif instruction_value == sp.ContextOpcode.OP_SET_UPVALUE:
            slot, vmRuntimeCallstack[-1].ip = read_byte(vm_runtime_read_only_main, vmRuntimeCallstack[-1])

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
        data_stack: typing.List, callee, arg_count: int) -> bool:
    
    #For native functions
    if isinstance(callee, types.FunctionType):
        result = callee(arg_count, [])
        del data_stack[len(data_stack) - (arg_count + 1):]
        data_stack.append(result)
        return True
    elif isinstance(callee, RuntimeClosure):
        new_function_address = callee.function_ptr
        if (new_function_address not in vm_runtime_read_only_main.contextmap):
            print("Can only call functions and classes")
        else:
            #Todo: Fix
            return call(vm_runtime_read_only_main, callee, arg_count, call_stack, data_stack)
    else:
        runtimeError("Invalid type", vm_runtime_read_only_main, call_stack)

def call(vm_runtime_read_only_main: VMRuntimeReadOnlyData, closure: RuntimeClosure, arg_count: int, call_stack: typing.List[CallStackSingleElement], data_stack: typing.List) -> bool:
    context = vm_runtime_read_only_main.contextmap[closure.function_ptr]
    if (arg_count != context.arity):
        # runtimeError(5, call_stack)
        runtimeError(f'Expected {context.arity} arguments but got {arg_count}', vm_runtime_read_only_main, call_stack)
        return False
    if (len(call_stack) > CALL_STACK_MAX):
        # print("Stack overflow")
        runtimeError("Stack overflow", vm_runtime_read_only_main, call_stack)
        return False
    call_stack.append(CallStackSingleElement(context.function_address, context.first_instruction_address, len(data_stack) -(arg_count + 1), closure.upvalues))
    return True

def main():
    if len(sys.argv) > 2 or len(sys.argv) < 2:
        print("Usage: plox [script]")
    elif len(sys.argv) == 2:
        (vm_runtime_read_only_main, vm_runtime_write_only_main, data_stack, initial_context_ptr) = generate_vm_data(sys.argv[1])
        closure = new_closure(vm_runtime_read_only_main.contextmap[initial_context_ptr])
        data_stack.pop()
        data_stack.append(closure)
        call(vm_runtime_read_only_main, closure, 0, vm_runtime_write_only_main.callstack, data_stack)
        run(vm_runtime_read_only_main, vm_runtime_write_only_main, data_stack)

if __name__ == "__main__":
    main()
