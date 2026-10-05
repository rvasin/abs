"""Shared fixtures for the abs test suite.

These are black-box integration tests: every test runs the real interpreter in a
subprocess and asserts on what it printed. There is deliberately no C++ unit
test target, because what needs protecting here is the *language* - the
evaluation order, the scoping rules, and the exact text the built-ins emit.
"""

import itertools
import os
import subprocess
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = Path(__file__).resolve().parent
CASES_DIR = TESTS_DIR / "cases"

# The interpreter emits "abs" on POSIX and "abs.exe" on Windows.
_BINARY_NAMES = ("abs.exe", "abs") if os.name == "nt" else ("abs", "abs.exe")

# Directories to search, relative to the repo root, for the built binary.
_SEARCH_DIRS = ("build", "build/Release", ".")


def _is_executable(path):
    return path.is_file() and os.access(path, os.X_OK)


def _find_binary():
    """Locate the interpreter, or return None."""
    from_env = os.environ.get("ABS_BIN")
    if from_env:
        path = Path(from_env)
        if _is_executable(path):
            return path
        raise pytest.UsageError(
            f"$ABS_BIN is set to {from_env!r}, which is not an executable file"
        )

    for directory in _SEARCH_DIRS:
        for name in _BINARY_NAMES:
            path = REPO_ROOT / directory / name
            if _is_executable(path):
                return path
    return None


def _child_env():
    """Environment for interpreter subprocesses.

    TZ and LC_ALL are pinned so that date built-ins and number formatting do
    not change with the developer's machine. Without this, a test asserting on
    strdate() passes in UTC and fails in CET.
    """
    env = dict(os.environ)
    env["TZ"] = "UTC"
    env["LC_ALL"] = "C"
    return env


@pytest.fixture(scope="session")
def abs_bin():
    """Absolute path to the built interpreter."""
    binary = _find_binary()
    if binary is None:
        pytest.skip(
            "interpreter binary not found. Build it first:\n"
            "    cmake --preset dev && cmake --build build\n"
            "or set $ABS_BIN to an existing binary."
        )
    return binary


class Result:
    """The outcome of running one abs script."""

    def __init__(self, script, proc):
        self.script = script
        self.proc = proc

    @property
    def out(self):
        return self.proc.stdout

    @property
    def code(self):
        return self.proc.returncode

    def __str__(self):
        return (
            f"<abs script>\n{self.script}\n"
            f"<exit code> {self.code}\n"
            f"<stdout> {self.out!r}\n"
            f"<stderr> {self.proc.stderr!r}"
        )


@pytest.fixture
def run(abs_bin, tmp_path):
    """Run abs source and return a Result.

    Usage::

        def test_x(run):
            r = run('print("hi\\n")')
            assert r.out == "hi\\n"
    """
    counter = itertools.count()

    def _run(source, *, args=(), stdin="", timeout=60, cwd=None, trailing=()):
        """Run a script.

        `args` are placed before the script name and `trailing` after it, so
        both flag orders can be exercised - abs accepts either.
        """
        script = tmp_path / f"case{next(counter)}.abs"
        script.write_text(source, encoding="utf-8")
        proc = subprocess.run(
            [str(abs_bin), *args, str(script), *trailing],
            input=stdin,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=str(cwd) if cwd else None,
            env=_child_env(),
        )
        return Result(source, proc)

    return _run


@pytest.fixture
def out(run):
    """Run abs source and return just stdout, asserting a clean exit."""

    def _out(source, **kwargs):
        result = run(source, **kwargs)
        assert result.code == 0, f"expected a clean exit\n{result}"
        return result.out

    return _out


@pytest.fixture
def run_repl(abs_bin):
    """Drive the interactive REPL over stdin.

    The REPL loops forever on EOF (see src/absmain.cpp:31), so `source` is
    always terminated with a literal `exit` line here. Do not add a REPL helper
    that omits it - the test would hang instead of failing.
    """

    def _run_repl(source, *, args=(), timeout=60):
        stdin = source if source.endswith("\n") else source + "\n"
        if not stdin.rstrip().endswith("exit"):
            stdin += "exit\n"
        proc = subprocess.run(
            [str(abs_bin), *args],
            input=stdin,
            capture_output=True,
            text=True,
            timeout=timeout,
            env=_child_env(),
        )
        return proc

    return _run_repl


def _discover_golden_cases():
    """Pair every cases/<name>.abs with its cases/<name>.expected."""
    if not CASES_DIR.is_dir():
        return []
    pairs = []
    for script in sorted(CASES_DIR.glob("*.abs")):
        expected = script.with_suffix(".expected")
        if expected.is_file():
            pairs.append((script.stem, script, expected))
    return pairs


GOLDEN_CASES = _discover_golden_cases()


@pytest.mark.parametrize(
    "name,script,expected",
    GOLDEN_CASES,
    ids=[case[0] for case in GOLDEN_CASES],
)
def test_golden_case(abs_bin, name, script, expected):
    """Run a whole script from tests/cases and compare stdout byte for byte."""
    proc = subprocess.run(
        [str(abs_bin), str(script)],
        capture_output=True,
        text=True,
        timeout=60,
        cwd=str(script.parent),
        env=_child_env(),
    )
    assert proc.returncode == 0, f"cases/{name}.abs exited {proc.returncode}"
    assert proc.stdout == expected.read_text(encoding="utf-8"), (
        f"cases/{name}.abs output drifted\n"
        f"  expected: {expected.read_text(encoding='utf-8')!r}\n"
        f"  actual:   {proc.stdout!r}"
    )
