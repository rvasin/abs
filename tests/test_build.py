"""Guards on the source tree itself rather than on language behaviour.

The interpreter is four translation units that share headers, and the only
thing holding them together is include order. That failed once: include/atoment.h
used `vector` without including <vector>, so it compiled only when some other
header happened to include it first. src/atoment.cpp - the one TU that includes
atoment.h before anything else - failed with ~24 errors while the rest of the
project built fine. Commit 941059c added the missing include; these tests stop
the same class of bug coming back.

Nothing here runs the interpreter, and both tests skip when the sources or a
compiler are absent, so the suite still passes against an installed binary with
no source tree alongside it.
"""

import shutil
import subprocess
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"
INCLUDE_DIR = REPO_ROOT / "include"

# The interpreter targets C++11, as configured in CMakeLists.txt. On MinGW
# run() needs popen, so the GNU dialect is required there; this mirrors the
# reasoning in CMakeLists.txt and docs/building.md.
STD_FLAG = "-std=gnu++11" if sys.platform == "win32" else "-std=c++11"


def _compiler():
    """Path to a C++ compiler, or None."""
    for name in ("g++", "clang++", "c++"):
        found = shutil.which(name)
        if found:
            return found
    return None


@pytest.fixture(scope="module")
def compiler():
    if not SRC_DIR.is_dir() or not INCLUDE_DIR.is_dir():
        pytest.skip("source tree not present; this is a source-checkout test")
    found = _compiler()
    if not found:
        pytest.skip("no C++ compiler found (looked for g++, clang++, c++)")
    return found


def _compile(compiler, source, include_dir, tmp_path):
    """Syntax-check one translation unit. Returns the CompletedProcess."""
    return subprocess.run(
        [compiler, STD_FLAG, f"-I{include_dir}", "-fsyntax-only", str(source)],
        capture_output=True,
        text=True,
        timeout=120,
        cwd=str(tmp_path),
    )


@pytest.mark.parametrize(
    "name", ["abs.cpp", "absmain.cpp", "atoment.cpp", "utils.cpp"]
)
def test_translation_unit_compiles_standalone(compiler, name, tmp_path):
    """Each .cpp must compile on its own, not only in a whole-program build.

    Building src/*.cpp together cannot catch this class of bug: the first
    translation unit to include a header supplies its missing includes for
    every unit after it. Only compiling one at a time does.
    """
    source = SRC_DIR / name
    if not source.is_file():
        pytest.skip(f"{name} not present")

    proc = _compile(compiler, source, INCLUDE_DIR, tmp_path)

    assert proc.returncode == 0, (
        f"{name} does not compile on its own.\n"
        f"It may be relying on another header to include something it uses.\n"
        f"--- stderr ---\n{proc.stderr}"
    )


def test_atoment_header_does_not_depend_on_include_order(compiler, tmp_path):
    """include/atoment.h must survive utils.h being included after the class.

    atoment.h used to write `vector` and `string` unqualified, so it needed the
    `using namespace std` that utils.h provides. That made the header's contents
    depend on an include appearing before them: move the include and the header
    stops parsing.

    Both types are now qualified (std::vector, std::string), so the order no
    longer matters. This rewrites the header with utils.h moved to the bottom
    and requires the result to compile.
    """
    atoment = INCLUDE_DIR / "atoment.h"
    utils_include = INCLUDE_DIR / "utils.h"
    if not (atoment.is_file() and utils_include.is_file()):
        pytest.skip("headers not present")

    staged = tmp_path / "include"
    staged.mkdir()
    for header in INCLUDE_DIR.glob("*.h"):
        shutil.copy(header, staged / header.name)

    target = staged / "atoment.h"
    lines = target.read_text(encoding="utf-8", newline="").splitlines(keepends=True)
    include_line = [l for l in lines if '#include "utils.h"' in l]
    assert len(include_line) == 1, (
        "expected exactly one #include \"utils.h\" in atoment.h, "
        f"found {len(include_line)} - update this test if that changed"
    )
    lines.remove(include_line[0])
    lines.append(include_line[0])  # re-insert below the class
    target.write_text("".join(lines), encoding="utf-8", newline="")

    source = SRC_DIR / "atoment.cpp"
    if not source.is_file():
        pytest.skip("atoment.cpp not present")

    proc = _compile(compiler, source, staged, tmp_path)

    assert proc.returncode == 0, (
        "atoment.h still depends on utils.h being included before its "
        "declarations.\n"
        "Every type it uses must be qualified (std::string, std::vector, ...), "
        "so include order cannot change whether the header parses.\n"
        f"--- stderr ---\n{proc.stderr}"
    )
