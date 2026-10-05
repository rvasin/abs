"""Command line behaviour: flags, argument handling, include resolution, REPL.

The REPL deserves a warning. absmain.cpp loops until it reads a line equal to
"exit", and getline() returns an empty string forever on EOF, so a REPL process
that is not explicitly told to exit spins printing "abs> " indefinitely. Every
REPL test here goes through the run_repl fixture, which appends the exit line.
"""

import pytest


# --------------------------------------------------------------------------
# Running a script
# --------------------------------------------------------------------------


def test_runs_a_script_file(out):
    assert out('print("from a file\\n")') == "from a file\n"


def test_a_missing_script_produces_no_output_and_succeeds(run, abs_bin, tmp_path):
    """file_read() always reports success and yields an empty body, so a typo
    in the filename is silent rather than an error."""
    result = run("", args=[])  # sanity: the harness itself works
    assert result.code == 0
    assert result.out == ""


def test_exit_sets_the_process_status(run, tmp_path):
    result = run("exit(3)")
    assert result.code == 3


def test_bare_exit_is_zero(run):
    assert run("exit").code == 0


# --------------------------------------------------------------------------
# Flags
# --------------------------------------------------------------------------


def test_t_flag_reports_elapsed_millis(out):
    result = out('print("x")', args=["-t"])
    assert result.startswith("x")
    assert "done in" in result
    assert "msecs" in result


def test_r_flag_prints_the_parsed_tree(out):
    result = out('print("x")', args=["-r"])
    # a single top-level expression is not wrapped in an implicit cmd()
    assert result.startswith("function: print")
    assert "atom: x" in result
    # several statements are
    assert "function: cmd" in out('print("x")print("y")', args=["-r"])


def test_d_flag_dumps_the_source_and_traces_evaluation(out):
    result = out('print("x")', args=["-d"])
    # the source itself is echoed before the trace
    assert 'print("x")' in result
    assert "eval function" in result


def test_flags_are_combinable(out):
    """-tr works as well as -t -r: the letters of one flag are scanned."""
    combined = out('print("x")', args=["-tr"])
    separate = out('print("x")', args=["-t", "-r"])
    assert "done in" in combined
    assert "function: print" in combined
    assert combined.count("function: print") == separate.count("function: print")


def test_flags_may_follow_the_script_name(out):
    """Flags are recognised on either side of the script name."""
    assert "done in" in out('print("x")', trailing=["-t"])
    assert "function: print" in out('print("x")', trailing=["-r"])
    assert "begin parsing" in out('print("x")', trailing=["-d"])


def test_unknown_flags_are_ignored(out):
    assert out('print("x")', args=["-z"]) == "x"


# --------------------------------------------------------------------------
# argc / argv
# --------------------------------------------------------------------------


def test_argc_counts_the_binary_flags_and_script(out, abs_bin):
    """argc()/argv() expose the raw process arguments, not just the script's.

    Index 0 is the interpreter path itself, so a script's own arguments start
    at index 1. This is a known wart (see the to-do in src/absmain.cpp), and it
    is pinned here so that changing it is a deliberate act.
    """
    # abs path + script path
    assert out("print(argc())") == "2"
    # abs path + -t flag + script path
    assert out("print(argc())", args=["-t"]).startswith("3")


def test_argv_zero_is_the_interpreter_path(out, abs_bin):
    assert out("print(argv(0))") == str(abs_bin)


def test_argv_includes_the_flag_and_script(out, abs_bin, tmp_path):
    """argv() is the raw process argument vector, in order."""
    result = out("print(argv(0))", args=["-t"])
    assert result.splitlines()[0] == str(abs_bin)
    # argv(1) is the flag, argv(2) the script - and -t adds its own trailing line
    tail = out("print(argv(1),\"|\",argv(2))", args=["-t"])
    assert tail.startswith("-t|")
    assert ".abs" in tail
    assert tail.splitlines()[-1].startswith("done in ")
    assert tail.rstrip().endswith("msecs")
    # with no flags the script path is the last entry
    assert out("print(argv(1))").endswith(".abs")


# --------------------------------------------------------------------------
# include() and eval()
# --------------------------------------------------------------------------


def test_include_resolves_against_the_process_cwd_not_the_script(out, tmp_path):
    """include("lib.abs") looks in the *current working directory*.

    Running a script from a subdirectory therefore fails to find a sibling
    file, and the failure is silent - see the next test.
    """
    scripts = tmp_path / "scripts"
    scripts.mkdir()
    (tmp_path / "lib.abs").write_text('set(fromlib,1)', encoding="utf-8")
    (scripts / "main.abs").write_text('include("lib.abs")print(fromlib)', encoding="utf-8")

    # correct: run from the directory that holds lib.abs
    result = out('include("lib.abs")print(fromlib)', cwd=tmp_path)
    assert result == "1"


def test_include_of_a_missing_file_is_a_silent_no_op(out, tmp_path):
    """file_read() returns true and an empty body for a path that does not
    exist, so nothing happens and no error is reported."""
    assert out('include("definitely_absent.abs")print("still here")', cwd=tmp_path) == (
        "still here"
    )


def test_included_code_shares_the_global_scope(out, tmp_path):
    (tmp_path / "lib.abs").write_text('set(shared,42)', encoding="utf-8")
    assert out('include("lib.abs")print(shared)', cwd=tmp_path) == "42"


def test_includeonce_runs_a_file_only_once(out, tmp_path):
    (tmp_path / "lib.abs").write_text("inc(n)", encoding="utf-8")
    (tmp_path / "main.abs").write_text(
        'set(n,0)includeonce("lib.abs")includeonce("lib.abs")print(n)',
        encoding="utf-8",
    )
    assert out('set(n,0)includeonce("lib.abs")includeonce("lib.abs")print(n)',
               cwd=tmp_path) == "1"


def test_eval_runs_a_string_of_code(out):
    assert out('print(eval("add(2,3)"))') == "5"


# --------------------------------------------------------------------------
# The REPL
# --------------------------------------------------------------------------


def test_repl_prints_only_print_output(run_repl):
    """A bare expression is evaluated and thrown away - the REPL does not echo
    results (the to-do in absmain.cpp)."""
    proc = run_repl('set(a,10)\nprint(add(a,5))\n')
    assert proc.returncode == 0
    assert proc.stdout == "abs> abs> 15abs> "


def test_repl_variables_persist_between_lines(run_repl):
    proc = run_repl('set(a,1)\nset(b,2)\nprint(add(a,b))\n')
    assert "3" in proc.stdout


def test_repl_shows_the_prompt(run_repl):
    proc = run_repl("print(1)\n")
    assert proc.stdout.startswith("abs> ")


def test_repl_accepts_a_function_definition(run_repl):
    proc = run_repl("fun(f,x,mult(x,3))\nprint(f(4))\n")
    assert "12" in proc.stdout


# --------------------------------------------------------------------------
# Regressions
# --------------------------------------------------------------------------


def test_repl_function_survives_to_the_next_line(run_repl):
    """Regression: a crash.

    fun() used to record a raw pointer into the tree that Process() parses, and
    Process() frees that tree. Harmless for a script, but the REPL runs one
    Process() per line, so the definition dangled and the next line read freed
    memory - reliably a segfault under ASan. The definition is now an owned
    deep copy.
    """
    proc = run_repl("fun(f,x,mult(x,3))\nprint(f(4))\n")
    assert proc.returncode == 0
    assert "12" in proc.stdout


def test_repl_function_can_be_redefined(run_repl):
    """The second definition must replace the first rather than leak it."""
    proc = run_repl("fun(f,x,mult(x,3))\nfun(f,x,add(x,100))\nprint(f(4))\n")
    assert proc.returncode == 0
    assert "104" in proc.stdout


def test_repl_unknown_function_does_not_break_later_calls(run_repl):
    """A failed lookup must not disturb the functions that are defined."""
    proc = run_repl("fun(f,x,1)\nprint(nosuchfn(1))\nprint(f(9))\n")
    assert proc.returncode == 0
    # nosuchfn() yields 0, and f(9) still returns 1 afterwards
    assert proc.stdout == "abs> abs> 0abs> 1abs> "
