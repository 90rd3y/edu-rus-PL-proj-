#include "parser_test.hpp"

// ============================================================================
//                               ENTRY POINT
// ============================================================================

int main() {
    TestRunner runner;

    std::cout << "--- Running Parser Unit Tests ---\n";

    runner.run("Include Statements", test_include_stmts);
    runner.run("Using Statements", test_using_stmts);
    runner.run("Namespace Statement", test_namespace_stmt);
    runner.run("Struct Declarations", test_struct_decls);
    runner.run("Function Declaration Signature", test_func_decl_signature);
    runner.run("Function Body Variable Declarations", test_func_body_var_decls);
    runner.run("Function Body Control Flow", test_func_body_control_flow);
    runner.run("Function Body Arrays and Spread",
               test_func_body_arrays_and_spread);
    runner.run("Function Body Slice and Cast", test_func_body_slice_and_cast);
    runner.run("Syntax Errors Rejection", test_syntax_errors);
    runner.run("Deep Unary Recursion", test_deep_unary_recursion);
    runner.run("Right Associative Chain of Assignments",
               test_right_associative_assignment);
    runner.run("Top Level Statement Volume", test_top_level_statement_volume);
    runner.run("Precedence Warfare", test_precedence_warfare);
    runner.run("Qualified function name in definition",
               test_qualified_func_name_in_def);
    runner.run("Qualified structure name in definition",
               test_qualified_struct_name_in_def);
    runner.run("Namespace in local scope", test_namespace_in_local_scope);
    runner.run("Assignment where expression expected",
               test_assignment_where_expr_expected);
    runner.run("Unnamed parameter in function definition",
               test_unnamed_param_in_func_def);
    runner.run("Must use for null type", test_must_use_for_null_type);
    runner.run("Optional static array", test_optional_static_array);
    runner.run("VLA forbidden", test_vla_forbidden);
    runner.run("Wrong spread operator appliance",
               test_wrong_spread_operator_appliance);
    runner.run("End of slice forgotten", test_end_of_slice_forgotten);

    runner.summary();

    return runner.failed > 0 ? 1 : 0;
}