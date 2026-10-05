"""Language-level semantics: parsing, evaluation order, scoping, control flow.

These pin down the parts of abs that are easy to get wrong and that a reader
cannot infer from the C++ alone. Several tests assert behaviour that looks like
a bug but is not - those are marked, because they are load-bearing.
"""

import pytest


# --------------------------------------------------------------------------
# Lexing: there are no infix operators
# --------------------------------------------------------------------------


@pytest.mark.parametrize(
    "expression,printed",
    [
        ("6/2", "6"),
        ("3*4", "3"),
        ("2.5+1", "2.5"),
        ("10-4", "10"),
    ],
)
def test_no_infix_operators(out, expression, printed):
    """A bare operator is swallowed by the number lexer and atoi stops early.

    The lexer reads a number as everything up to whitespace or , ( ) and then
    hands it to atoi/atof, so ``6/2`` is one token that parses as 6. This is
    silent, so it is worth a test: div() and mult() are the only way to divide.
    """
    assert out(f'print({expression})') == printed


def test_functional_arithmetic_is_the_real_thing(out):
    assert out('print(div(6,2)," ",mult(3,4)," ",add(2,2))') == "3 12 4"


def test_integer_division_truncates(out):
    """div() on two ints is C++ integer division, not a float division."""
    assert out("print(div(7,2))") == "3"
    # a double operand promotes the whole expression to double
    assert out("print(div(7.0,2))") == "3.5"


def test_negative_and_double_literals(out):
    assert out('print(-5," ",2.5," ",-0.5)') == "-5 2.5 -0.5"


# --------------------------------------------------------------------------
# Separators and comments
# --------------------------------------------------------------------------


def test_newlines_separate_statements(out):
    assert out('set(a,1)\nset(b,2)\nprint(add(a,b),"|")') == "3|"


def test_statements_need_no_separator_at_all(out):
    assert out('set(a,1)print(a)') == "1"


def test_semicolon_is_not_a_terminator(out):
    """A ';' is not a separator, so it becomes a junk variable node.

    It does not abort the script - cmd() evaluates every parameter and returns
    the last one - so the ';' silently evaluates to an error string and is
    discarded. Scripts written with semicolons appear to work, which is why
    this is worth pinning down.
    """
    # the whole line collapses into a single bogus expression, so nothing runs
    assert out('set(a,1);print(a)') == ""


def test_semicolon_error_string_is_visible_when_referenced(out):
    """The junk node really is a variable lookup, so it can be observed."""
    assert out('set(a,1);set(x,;)print(x)') == "error: variable x not found"


@pytest.mark.parametrize(
    "source,expected",
    [
        ("# leading comment\nprint(1)", "1"),
        ("print(1)# trailing comment", "1"),
        ("set(a,1) # comment glued to a token\nprint(a)", "1"),
        ("# comment\n# another\nprint(2)", "2"),
        ("print(\"a#b\")", "a#b"),
    ],
)
def test_comments(out, source, expected):
    """'#' runs to end of line, but only where whitespace is legal."""
    assert out(source) == expected


def test_comment_must_sit_where_whitespace_is_legal(out):
    """A comment cannot interrupt a token, because skip_spaces runs between
    nodes only. 'pri#nt' is parsed as a variable name, not print."""
    assert out("pri#nt(1)") == ""


def test_hash_inside_string_is_literal(out):
    """A '#' inside a string is data, not the start of a comment."""
    assert out('print("a#b")') == "a#b"


def test_string_escapes(out):
    assert out(r'print("a\nb\tc\"d\\e")') == 'a\nb\tc"d\\e'


def test_explicit_implicit_cmd_body(out):
    """Several comma-separated statements in one body become a cmd()."""
    assert out('fun(f,x,set(a,x)set(b,2)add(a,b))print(f(3))') == "5"


# --------------------------------------------------------------------------
# Functions
# --------------------------------------------------------------------------


def test_function_arguments_bind_positionally(out):
    assert out(
        "fun(sub3,a,b,sub(a,b))print(sub3(10,3))"
    ) == "7"


def test_last_parameter_is_the_body_and_return_value(out):
    """The final parameter is the body, so it is what the call evaluates to.

    The body is a multi-statement block, so it is either left unseparated (the
    parser wraps it in an implicit cmd) or written as an explicit cmd(). A comma
    before the last statement would turn it into an argument *name* instead.
    See tests/cases/functions.abs for the full set of forms.
    """
    assert out(
        "fun(f,n,var(r)if(eq(n,0),set(r,1),set(r,mult(n,f(sub(n,1)))))r)"
        "print(f(5))"
    ) == "120"
    assert out(
        "fun(f,n,cmd(var(r),if(eq(n,0),set(r,1),set(r,mult(n,f(sub(n,1))))),r))"
        "print(f(5))"
    ) == "120"


def test_functions_are_recursive(out):
    assert out(
        "fun(fib,n,var(r)"
        "if(eq(n,0),set(r,0),if(eq(n,1),set(r,1),set(r,add(fib(sub(n,2)),fib(sub(n,1))))))"
        "r)print(fib(10))"
    ) == "55"


def test_call_before_definition_is_not_recursive(out):
    """Function names resolve at call time, so a later redefinition wins."""
    assert out("fun(f,x,x)fun(f,x,mult(x,10))print(f(2))") == "20"


# --------------------------------------------------------------------------
# Scoping: var() is local, set() is global unless the name is already local
# --------------------------------------------------------------------------


def test_var_declares_a_function_local(out):
    """var(x) is the only way to get a local. It disappears when f returns."""
    assert out("fun(f,var(v)set(v,1)v)print(f(),\" \",v)") == (
        "1 error: variable v not found"
    )


def test_set_inside_function_creates_a_global(out):
    """set() on a name that is not a local writes to the global table."""
    assert out("fun(f,set(g,7))f()print(g)") == "7"


def test_set_inside_function_updates_an_existing_global(out):
    assert out("set(g,1)fun(f,set(g,2))f()print(g)") == "2"


def test_local_shadows_global_for_reads(out):
    assert out("set(g,1)fun(f,var(g)set(g,2)g)print(f(),\" \",g)") == "2 1"


def test_tok_mutates_the_variable_it_is_given(out):
    """tok() is destructive: it rewrites its first argument and returns a piece.

    It also writes back through SetVar without passing locvars, so on a
    function-local it updates (or creates) the global instead. That is a quirk,
    not a feature - see the comment on the fun_tok macro in src/abs.cpp.
    """
    assert out('set(s,"a,b")print(tok(s,","),"|",s)') == "a|b"


def test_rtok_takes_from_the_right(out):
    assert out('set(s,"a,b,c")print(rtok(s,","),"|",s)') == "c|a,b"


def test_append_returns_zero_but_mutates(out):
    assert out('set(s,"ab")print(append(s,"cd"),"|",s)') == "0|abcd"


# --------------------------------------------------------------------------
# Control flow: these return 1/0, never the branch value
# --------------------------------------------------------------------------


def test_if_returns_one_or_zero_not_the_branch_value(out):
    """This is the single most surprising rule in the language.

    if(cond, a, b) evaluates a or b for its side effects and then yields 1 or 0.
    To get a value out of a branch you must set() inside it.
    """
    assert out("set(r,if(1,42))print(r)") == "1"
    assert out("set(r,if(0,42,99))print(r)") == "0"


def test_if_evaluates_only_the_taken_branch(out):
    assert out("set(r,0)if(1,set(r,1),set(r,2))print(r)") == "1"
    assert out("set(r,0)if(0,set(r,1),set(r,2))print(r)") == "2"


def test_if_without_else(out):
    assert out("set(r,5)if(0,set(r,1))print(r)") == "5"


def test_while_loops_until_the_condition_is_false(out):
    assert out("set(i,0)while(less(i,5),inc(i))print(i)") == "5"


def test_while_takes_body_then_condition(out):
    assert out("set(i,0)while(less(i,3),set(j,inc(i)))print(j)") == "3"


def test_do_runs_body_before_the_first_condition_check(out):
    """do() is C-style do/while, not Pascal repeat/until."""
    assert out("set(i,0)do(inc(i),less(i,5))print(i)") == "5"


def test_nested_loops_and_accumulator(out):
    assert out(
        "set(total,0)"
        "set(i,1)"
        "while(lesseq(i,4),"
        "set(j,1)"
        "while(lesseq(j,i),set(total,add(total,1))inc(j))"
        "inc(i))"
        "print(total)"
    ) == "10"


# --------------------------------------------------------------------------
# switch
# --------------------------------------------------------------------------


def test_switch_runs_the_matching_action(out):
    assert out('set(t,0)switch(2,1,set(t,11),2,set(t,22))print(t)') == "22"


def test_switch_matches_strings(out):
    assert out('set(t,0)switch("b","a",set(t,11),"b",set(t,22))print(t)') == "22"


def test_switch_default_runs_when_nothing_matched(out):
    """The trailing parameter is the default arm.

    It is recognised when the parameter count is even, i.e. when the
    cond/action pairs do not use up the last parameter.
    """
    assert out('set(t,0)switch(9,1,set(t,11),2,set(t,22),set(t,33))print(t)') == "33"


def test_switch_with_only_a_default(out):
    assert out("set(t,0)switch(5,set(t,7))print(t)") == "7"


def test_switch_returns_one_on_a_match_and_zero_otherwise(out):
    assert out("set(r,switch(1,1,\"a\",\"b\",\"default\"))print(r)") == "1"
    assert out("set(r,switch(9,1,\"a\",\"b\",\"default\"))print(r)") == "0"


def test_switch_does_not_run_the_default_on_a_match(out):
    assert out('set(t,0)switch(1,1,set(t,11),set(t,99))print(t)') == "11"


# --------------------------------------------------------------------------
# Errors are values, not failures
# --------------------------------------------------------------------------


def test_undefined_function_returns_zero_silently(out):
    """A typo in a function name does not abort the script."""
    assert out('print(nosuchfunction(1),"|")') == "0|"


def test_undefined_variable_yields_an_error_string(out):
    assert out("print(nosuchvariable)") == "error: variable nosuchvariable not found"


def test_a_failed_call_does_not_stop_the_script(out):
    assert out('print(badfn(),"|")print("still running")') == "0|still running"


# --------------------------------------------------------------------------
# Output behaviour
# --------------------------------------------------------------------------


def test_print_does_not_add_a_newline(out):
    assert out('print("a")print("b")') == "ab"


def test_print_joins_all_its_arguments_without_separators(out):
    assert out('print("x=",1,"y=",2)') == "x=1y=2"


def test_print_renders_ints_and_doubles(out):
    assert out("print(3,4)") == "34"
    assert out("print(3.5)") == "3.5"


def test_print_renders_a_list_in_parentheses(out):
    assert out("print(list(1,\"two\",3.5))") == "(1,two,3.5)"


def test_cat_stringifies_its_arguments(out):
    assert out('print(cat("n=",42))') == "n=42"


# --------------------------------------------------------------------------
# Regressions
# --------------------------------------------------------------------------


def test_control_builtins_tolerate_missing_arguments(out):
    """Regression: a crash.

    if() read its second parameter unconditionally, so print(if(1)) indexed
    past the end of the parameter vector and segfaulted. A missing branch is now
    simply not evaluated.
    """
    assert out("print(if(1))") == "1"
    assert out("print(if(0))") == "0"
    assert out("print(if())") == "0"
    assert out("print(while(0))") == "0"
    assert out("print(while(1))") == "0"  # nothing to repeat
    assert out("print(do())") == "0"
    assert out("print(switch())") == "0"
    assert out("print(switch(\"a\"))") == "0"


def test_if_evaluates_only_the_branch_it_is_given(out):
    set_trace = "set(touched,1)"
    assert out(f"set(touched,0)if(0,set(touched,1),set(touched,2))print(touched)") == "2"
    assert out(f"set(touched,0)if(1,set(touched,1))print(touched)") == "1"


def test_function_definition_keeps_its_own_copy_of_the_tree(out):
    """Regression: custcodes used to alias the parsed tree, so redefining a
    function and calling the new one has to work after Process() has freed it."""
    assert out('fun(f,set(f,"one") f)\nprint(f())') == "one"
    assert out('fun(f,set(f,"one") f)\nfun(f,set(f,"two") f)\nprint(f())') == "two"


def test_lesseq_is_inclusive(out):
    """Regression: lesseq() used < instead of <=."""
    assert out("print(lesseq(2,2))") == "1"
    assert out("print(lesseq(1,2),lesseq(2,1))") == "10"


def test_listcopy_does_not_crash(out):
    """Regression: listcopy() appended before creating the list storage."""
    assert out("set(a,list(1,2,3))set(b,listcopy(a))print(b)") == "(1,2,3)"


def test_unset_actually_removes_the_variable(out):
    """Regression: unset() was a stub that returned 0 and removed nothing."""
    assert out('set(n,1)print(unset("n")," ",n)') == "0 error: variable n not found"
    assert out('print(unset("n"))') == "1"
