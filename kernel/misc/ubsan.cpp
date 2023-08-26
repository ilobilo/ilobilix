// Copyright (C) 2022-2023  ilobilo

#include <misc/ubsan.hpp>
#include <arch/arch.hpp>
#include <lib/trace.hpp>
#include <lib/log.hpp>

static void print(const char *message, source_location loc)
{
    log::warnln("Ubsan: {} at file {}, line {}, column {}", message, loc.file, loc.line, loc.column);
    // trace::print(reinterpret_cast<uintptr_t>(__builtin_frame_address(1)));
    arch::halt(false);
}

extern "C"
{
    void __ubsan_handle_add_overflow(overflow_data *data)
    {
        print("addition overflow", data->location);
    }

    void __ubsan_handle_sub_overflow(overflow_data *data)
    {
        print("subtraction overflow", data->location);
    }

    void __ubsan_handle_mul_overflow(overflow_data *data)
    {
        print("multiplication overflow", data->location);
    }

    void __ubsan_handle_divrem_overflow(overflow_data *data)
    {
        print("division overflow", data->location);
    }

    void __ubsan_handle_negate_overflow(overflow_data *data)
    {
        print("negation overflow", data->location);
    }

    void __ubsan_handle_pointer_overflow(overflow_data *data)
    {
        print("pointer overflow", data->location);
    }

    void __ubsan_handle_shift_out_of_bounds(shift_out_of_bounds_data *data)
    {
        print("shift out of bounds", data->location);
    }

    void __ubsan_handle_load_invalid_value(invalid_value_data *data)
    {
        print("invalid load value", data->location);
    }

    void __ubsan_handle_out_of_bounds(array_out_of_bounds_data *data)
    {
        print("array out of bounds", data->location);
    }

    void __ubsan_handle_type_mismatch_v1(type_mismatch_v1_data *data, uintptr_t ptr)
    {
        if (ptr == 0)
            print("use of nullptr", data->location);
        else if (ptr & ((1 << data->log_alignment) - 1))
            print("unaligned access", data->location);
        else
            print("no space for object", data->location);
    }

    void __ubsan_handle_function_type_mismatch(function_type_mismatch_v1_data *data, uintptr_t ptr)
    {
        print("call to a function through pointer to incorrect function", data->location);
    }

    void __ubsan_handle_function_type_mismatch_v1(function_type_mismatch_v1_data *data, uintptr_t ptr, uintptr_t calleeRTTI, uintptr_t fnRTTI)
    {
        print("call to a function through pointer to incorrect function", data->location);
    }

    void __ubsan_handle_vla_bound_not_positive(negative_vla_data *data)
    {
        print("variable-length argument is negative", data->location);
    }

    void __ubsan_handle_nonnull_return(nonnull_return_data *data)
    {
        print("non-null return is null", data->location);
    }

    void __ubsan_handle_nonnull_return_v1(nonnull_return_data *data)
    {
        print("non-null return is null", data->location);
    }

    void __ubsan_handle_nonnull_arg(nonnull_arg_data *data)
    {
        print("non-null argument is null", data->location);
    }

    void __ubsan_handle_builtin_unreachable(unreachable_data *data)
    {
        print("unreachable code reached", data->location);
    }

    void __ubsan_handle_invalid_builtin(invalid_builtin_data *data)
    {
        print("invalid builtin", data->location);
    }

    void __ubsan_handle_float_cast_overflow(float_cast_overflow_data *data)
    {
        print("float cast overflow", data->location);
    }

    void __ubsan_handle_missing_return(missing_return_data *data)
    {
        print("missing return", data->location);
    }

    void __ubsan_handle_alignment_assumption(alignment_assumption_data *data)
    {
        print("alignment assumption", data->location);
    }
} // extern "C"