"""Built-in function behaviour, grouped by area.

Every function listed in the language reference has coverage here. The
regression tests at the top correspond to bugs that were found and fixed while
setting this suite up; each one names the defect so a future regression is easy
to identify.
"""

import pytest

# 2021-01-02 03:04:05 UTC - a fixed instant so date assertions never race
# against the wall clock. conftest forces TZ=UTC.
STAMP = 1609556645


# --------------------------------------------------------------------------
# Regressions
# --------------------------------------------------------------------------


def test_lesseq_is_inclusive(out):
    """Regression: lesseq() was implemented with '<', so lesseq(2,2) was 0."""
    assert out("print(lesseq(2,2),lesseq(1,2),lesseq(3,2))") == "110"
    assert out("print(less(2,2),greq(2,2))") == "01"


def test_listcopy_does_not_crash_and_is_independent(out):
    """Regression: listcopy() appended before allocating its list storage, so
    AddListElem dereferenced a null vector and the interpreter segfaulted."""
    assert out("set(a,list(1,2,3))set(b,listcopy(a))print(b)") == "(1,2,3)"
    # a copy must not alias the original
    assert out(
        "set(a,list(1,2))set(b,listcopy(a))listappend(b,3)print(a,\"|\",b)"
    ) == "(1,2)|(1,2,3)"


def test_unset_removes_the_variable(out):
    """Regression: unset() was a stub that returned 0 and freed nothing."""
    assert out('set(x,42)print(unset("x"),"|",x)') == (
        "0|error: variable x not found"
    )


def test_unset_reports_whether_the_variable_existed(out):
    """0 when it removed something, 1 when there was nothing to remove."""
    assert out('set(x,1)print(unset("x")," ",unset("x"))') == "0 1"
    assert out('print(unset("neverexisted"))') == "1"


def test_unset_takes_a_string_name_not_a_value(out):
    """unset() takes the *name* as a string, unlike get() and set().

    Passing the variable itself does not unset it: the value 1 is looked up as
    a name, fails, and unset() reports 1 while leaving the variable alone.
    """
    assert out('set(n,1)print(unset("n")," ",n)') == "0 error: variable n not found"
    assert out('set(n,1)print(unset(n)," ",n)') == "1 1"


def test_unset_of_a_local_does_not_touch_a_same_named_global(out):
    """Regression guard for the scoping of unset().

    unset() looks in the local table first, and must erase from whichever map
    actually owns the variable - otherwise removing a local would also drop a
    global that happens to share the name. Once the local is gone the name
    falls back to the global, which is why f() returns the global's 999 rather
    than an error, and the global still holds 999 afterwards.
    """
    assert out(
        "set(dup,999)"
        "fun(f,var(dup)set(dup,1)unset(\"dup\")set(res,dup)res)"
        "print(f(),\" \",dup)"
    ) == "999 999"


def test_unset_of_a_list(out):
    assert out('set(l,list(1,2))print(unset("l")," ",l)') == (
        "0 error: variable l not found"
    )


def test_switch_default_arm_is_reachable(out):
    """Regression: the default guard was 'pcount / 2 == 0', which is never
    true for a well-formed switch, so the default arm was dead code."""
    assert out("set(t,0)switch(9,1,set(t,11),2,set(t,22),set(t,33))print(t)") == "33"


def test_trimright_of_an_all_space_string(out):
    """Regression: strtrimright() walked an unsigned index past 0, wrapping to
    SIZE_MAX and reading one byte before the string's buffer - a
    heap-buffer-overflow that happened to give the right answer.

    The string is built long enough to be heap allocated (past the small-string
    optimisation) so that the out-of-bounds read is actually observable.
    """
    long_spaces = 'cat("%s","%s")' % (" " * 20, " " * 20)
    assert out(f'set(s,{long_spaces})print("[",trimright(s),"]")') == "[]"
    assert out(f'set(s,{long_spaces})print("[",trim(s),"]")') == "[]"
    assert out('print("[",trimright(""),"]")') == "[]"


def test_daysinmonth_rejects_an_out_of_range_month(out):
    """Regression: GetDaysInMonth() indexed a 12-element array with the month
    unchecked, so daysinmonth(2024,13) read past the end."""
    assert out("print(daysinmonth(2024,2),daysinmonth(2023,2))") == "2928"
    assert out("print(daysinmonth(2024,0),daysinmonth(2024,13),daysinmonth(0,0))") == (
        "000"
    )


# --------------------------------------------------------------------------
# Arithmetic
# --------------------------------------------------------------------------


def test_add_and_mult_promote_to_double_on_demand(out):
    assert out("print(add(1,2),add(1,2.5),mult(2,3),mult(2,0.5))") == "33.561"


def test_mod_and_sign(out):
    assert out("print(mod(7,3),sign(-4),sign(0),sign(9))") == "1-101"


def test_inc_and_dec_take_an_optional_step(out):
    assert out("set(a,1)print(inc(a),\"|\",a)") == "2|2"
    assert out("set(a,1)print(inc(a,5),\"|\",a)") == "6|6"
    assert out("set(a,1)print(dec(a),\"|\",a)") == "0|0"
    assert out("set(a,1)print(dec(a,5),\"|\",a)") == "-4|-4"


def test_rounding_family(out):
    assert out("print(round(2.6),round(2.4),trunc(2.9),floor(2.9),ceil(2.1))") == (
        "32223"
    )


def test_root_and_power(out):
    assert out("print(sqrt(16),sqr(4),pow(2,10),abs(-3),frac(2.5))") == "416102430.5"


def test_min_and_max_vary_arity(out):
    assert out("print(max(3,7,1),\"|\",min(3,7,1))") == "7|1"


def test_trigonometry_at_the_quarter_turns(out):
    assert out("print(sin(0),cos(0),tan(0))") == "010"


def test_logarithms_and_exponentials(out):
    assert out("print(log(1),log10(100),exp(0))") == "021"


def test_pi_and_e_take_no_arguments(out):
    """pi and e are functions, not bare constants - pi() not pi."""
    assert out("print(pi())") == "3.14159"
    assert out("print(e())") == "2.71828"
    assert out("print(pi)") == "error: variable pi not found"


# --------------------------------------------------------------------------
# Comparison and logic
# --------------------------------------------------------------------------


def test_eq_compares_across_int_and_double(out):
    assert out("print(eq(1,1),eq(1,1.0),eq(1,2),noteq(1,2))") == "1101"


def test_eq_on_strings(out):
    assert out('print(eq("a","a"),eq("a","b"))') == "10"


def test_and_or_not(out):
    assert out("print(and(1,1),and(1,0),or(0,1),or(0,0),not(0),not(5))") == (
        "101010"
    )


def test_and_short_circuits(out):
    """and() stops at the first false argument, so the side effect is skipped."""
    assert out("set(t,0)and(0,set(t,1))print(t)") == "0"
    assert out("set(t,0)and(1,set(t,1))print(t)") == "1"


def test_comparison_family(out):
    assert out("print(less(1,2),lesseq(2,2),gr(2,1),greq(2,2))") == "1111"
    assert out("print(less(2,2),lesseq(3,2),gr(1,2),greq(1,2))") == "0000"


# --------------------------------------------------------------------------
# Strings
# --------------------------------------------------------------------------


def test_len_and_pos(out):
    """pos() is (haystack, needle[, start]) and returns -1 when absent."""
    assert out('print(len("hello")," ",pos("hello","llo")," ",pos("hello","zz"))') == (
        "5 2 -1"
    )
    assert out('print(pos("hello","llo",3))') == "-1"


def test_copy_erase_insert_replace(out):
    assert out('print(copy("hello",1,3))') == "ell"
    assert out('print(erase("hello",1,2))') == "hlo"
    assert out('print(insert("hello",1,"XY"))') == "hXYello"
    assert out('print(replace("hello","l","L"))') == "heLLo"


def test_treplace_mutates_a_variable_and_returns_the_count(out):
    """treplace() is the in-place variant, like tok() and append().

    Its first parameter is a *variable name*, not a string. Passing a literal
    does not error: the literal is looked up as a variable, comes back as
    "error: variable ... not found", and gets edited - which also creates a
    junk global. Always pass a real variable.
    """
    assert out('set(s,"aaa")print(treplace(s,"a","b")," ",s)') == "3 bbb"
    # "banana" has three a's
    assert out('set(s,"banana")print(treplace(s,"a","o")," ",s)') == "3 bonono"


def test_treplace_with_a_literal_creates_a_junk_variable(out):
    """Documents the footgun above, so the behaviour is not a surprise later.

    "ab" is looked up as a variable, fails, and the resulting error string
    "error: variable ab not found" is what gets edited - it happens to contain
    three "a" characters, hence the count. A junk global named "ab" is created
    as a side effect.
    """
    assert out('print(treplace("ab","a","z"))') == "3"


def test_upper_and_lower(out):
    assert out('print(upper("ab"),lower("AB"))') == "ABab"


def test_trim_family(out):
    assert out('print("[",trim("  x  "),"]")') == "[x]"
    assert out('print("[",trimleft("  x  "),"]")') == "[x  ]"
    assert out('print("[",trimright("  x  "),"]")') == "[  x]"


def test_char_str_int_float_conversions(out):
    assert out('print(char(65),str(12),int("34"),float("1.5"))') == "A12341.5"


def test_format_returns_the_formatted_string(out):
    assert out('print(format("%05.2f|%d|%s",3.14159,7,"z"))') == "03.14|7|z"
    assert out('print(format("%d items",3))') == "3 items"


def test_printf_writes_to_stdout_and_returns_zero(out):
    """printf() is the printing sibling of format(), not an alias: it writes
    straight to stdout and evaluates to 0, so wrapping it in print() shows the
    stray 0."""
    assert out('printf("%05.2f",3.14159)') == "03.14"
    assert out('print(printf("%05.2f",3.14159))') == "03.140"
    assert out('set(n,printf("hello"))print(n)') == "hello0"


def test_cat_concatenates_and_stringifies(out):
    assert out('print(cat("a",1,"b"))') == "a1b"


def test_env_of_an_unset_variable_is_empty(out):
    assert out('print("[",env("ABS_DEFINITELY_NOT_SET_12345"),"]")') == "[]"


# --------------------------------------------------------------------------
# Lists
# --------------------------------------------------------------------------


def test_list_construction_and_size(out):
    assert out("set(l,list(1,2,3))print(l,\"|\",listsize(l))") == "(1,2,3)|3"


def test_nested_lists(out):
    assert out("print(list(list(1,2),list(3,4)))") == "((1,2),(3,4))"
    assert out("set(l,list(list(1,2),3))print(listsize(l),\" \",listget(l,0))") == (
        "2 (1,2)"
    )


def test_listget_and_listset(out):
    assert out("set(l,list(1,2,3))print(listget(l,1))") == "2"
    assert out("set(l,list(1,2,3))print(listset(l,1,9),\"|\",l)") == "0|(1,9,3)"


def test_listappend_insert_remove(out):
    assert out("set(l,list(1,2))print(listappend(l,3),\"|\",l)") == "0|(1,2,3)"
    assert out("set(l,list(1,2))print(listinsert(l,0,0),\"|\",l)") == "0|(0,1,2)"
    assert out("set(l,list(1,2,3))print(listremove(l,1),\"|\",l)") == "0|(1,3)"


def test_list_mutators_return_zero(out):
    """They report success, and you observe the change through the variable."""
    assert out("set(l,list(1))print(listappend(l,2),listinsert(l,0,0),listremove(l,0))") == (
        "000"
    )


# --------------------------------------------------------------------------
# Variables
# --------------------------------------------------------------------------


def test_get_reads_a_variable_by_name(out):
    assert out('set(v,7)print(get("v"))') == "7"


def test_set_and_get_via_computed_names(out):
    """The name may be an expression, which is how you fake arrays.

    Note there is no infix arithmetic in abs: add(x0,x1), never x0+x1. The
    lexer would read "x0+x1" as one identifier and quietly produce an error
    string.
    """
    assert out('set(cat("x",0),10)set(cat("x",1),20)print(add(x0,x1))') == "30"
    assert out('set(cat("x",0),10)set(cat("x",1),20)print(x0+x1)') == (
        "error: variable x0+x1 not found"
    )


# --------------------------------------------------------------------------
# Date and time
# --------------------------------------------------------------------------


def test_date_builds_a_timestamp_from_parts(out):
    """date() is a constructor, not a getter: date(y,m,d,h,min,s) -> timestamp.

    The date-part functions go the other way (year(t), month(t), ...).
    conftest pins TZ=UTC so the value is stable across machines.
    """
    # 2021-01-02 03:04:05 UTC
    assert out("print(date(2021,1,2,3,4,5))") == "1609556645"
    assert out("print(strdate(date(2021,1,2,3,4,5)))") == "2021-01-02 03:04:05"


def test_date_parts_of_a_fixed_timestamp(out):
    assert out(f"print(strdate({STAMP}))") == "2021-01-02 03:04:05"
    assert out(f"print(year({STAMP}),month({STAMP}),day({STAMP}))") == "202112"
    assert out(f"print(hour({STAMP}),minute({STAMP}),second({STAMP}))") == "345"


def test_date_with_missing_arguments_is_still_deterministic(out):
    """Regression: the struct tm behind date() used to be left uninitialised,
    so omitted fields (notably tm_isdst) were read from indeterminate memory.
    It is now value-initialised, so the omitted time-of-day is midnight."""
    assert out("print(strdate(date(2021,1,2)))") == "2021-01-02 00:00:00"


def test_timestamps_are_32_bit(out):
    """Documents a real limitation rather than asserting a wish.

    date() and now() truncate time_t to a signed 32-bit int, so the usable
    range is roughly 1901-2038 and pre-1970 dates wrap to a wrong value.
    """
    assert out("print(less(now(),2000000000))") == "1"
    # a literal beyond INT_MAX does not survive: 4102444800 arrives as -192522496
    assert out("print(4102444800)") == "-192522496"


def test_difftime(out):
    assert out("print(difftime(100,50))") == "50"


def test_calendar_helpers(out):
    assert out("print(isleapyear(2024),isleapyear(2023))") == "10"
    assert out("print(daysinyear(2024),daysinyear(2023))") == "366365"
    assert out("print(daysinmonth(2024,2),daysinmonth(2023,2),daysinmonth(2024,1))") == (
        "292831"
    )


def test_now_is_close_to_the_current_epoch(out):
    """Coarse on purpose: this only guards against a wildly wrong clock."""
    assert out("set(t,now())print(less(t,2000000000))") == "1"


# --------------------------------------------------------------------------
# Files and shell
# --------------------------------------------------------------------------


def test_file_roundtrip_and_exists(out, tmp_path):
    (tmp_path / "data.txt").write_text("hello", encoding="utf-8")
    assert out('print(fileread("data.txt"),"|",fileexists("data.txt"))', cwd=tmp_path) == (
        "hello|1"
    )
    assert out('print(fileexists("nope.txt"))', cwd=tmp_path) == "0"


def test_filewrite_then_fileappend(out, tmp_path):
    out('filewrite("f.txt","one")', cwd=tmp_path)
    out('fileappend("f.txt","-two")', cwd=tmp_path)
    assert out('print(fileread("f.txt"))', cwd=tmp_path) == "one-two"


def test_run_captures_stdout(out):
    """run() shells out and returns what the command printed, trailing newline
    and all - unlike format()/printf(), it does not interpret escape sequences."""
    assert out('print(run("echo hello"),"|")') == "hello\n|"


def test_sys_returns_a_wait_status(out):
    """sys() returns system()'s raw wait status, not a normalised exit code.

    "exit 3" becomes 3 << 8 == 768, and a signal death is 128 + signal. Compare
    with gr() rather than using infix '>=' - abs has no infix operators.
    """
    assert out('print(sys("exit 3"))') == "768"
    assert out('print(sys("exit 0"))') == "0"
    assert out('print(gr(sys("exit 3"),0))') == "1"
