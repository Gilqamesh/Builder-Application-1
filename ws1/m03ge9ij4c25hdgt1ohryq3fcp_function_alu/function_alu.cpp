#include <m03ge9ij4c25hdgt1ohryq3fcp_function_alu/function_alu.h>
#include <m03ge9ij4azvq47860wqjyzskd_function_primitive_lang/function_primitive_lang.h>
#include <iostream>
#include <stdexcept>

namespace m03ge9ij4c25hdgt1ohryq3fcp_function_alu {

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::add(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "add", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a + b);
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::sub(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "sub", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a - b);
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::mul(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "mul", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a * b);
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::div(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "div", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int a = function.read(0);
        int b = function.read(1);
        function.write(2, a / b);
        if (b == 0) {
            // TODO: better error type
            throw std::runtime_error("division by zero");
        }
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::cond(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "cond", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        bool condition = function.read(0);
        if (condition) {
            function.copy(1, 3);
        } else {
            function.copy(2, 3);
        }
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::is_zero(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "is_zero", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int in = function.read(0);

        function.write(1, in == 0);
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::integer(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "integer", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        function.write(0, 0);
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::logger(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "logger", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        (void) argument_index;

        int in = function.read(0);
        try {
            std::ostream* os = function.read(1);
            *os << in << "\n";
        } catch (...) {
            std::cout << in << "\n";
        }
    });
}

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_alu_t::pin(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem) {
    return m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t::function(typesystem, "function_alu", "pin", [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
        for (size_t i = 0; i < function.arguments().size(); ++i) {
            if (i != argument_index && function.is_connected(i)) {
                function.copy(argument_index, i);
            }
        }
    });
}

} // namespace m03ge9ij4c25hdgt1ohryq3fcp_function_alu
