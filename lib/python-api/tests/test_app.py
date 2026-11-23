"""
Test the clingcon application.
"""

import json
import subprocess
import sys


def run_app(*args: str, program: str = "") -> subprocess.CompletedProcess[str]:
    """
    Run the application in a separate process.

    The application cannot run in the test process because it reads the
    command line from sys.argv, exits the process, and might exit on the C
    level.
    """
    return subprocess.run(
        [sys.executable, "-c", "import clingcon; clingcon._pyclingcon()", *args],
        input=program,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def test_version():
    """
    Test printing the version.
    """
    res = run_app("--version")
    assert res.returncode == 0
    assert res.stdout.startswith("clingcon version ")


def test_sat():
    """
    Test solving a satisfiable program.
    """
    res = run_app("--outf=2", "0", program="&dom { 1..2 } = x. &sum { x } >= 2. {p}.")
    assert res.returncode == 30
    out = json.loads(res.stdout)
    assert out["Result"] == "SATISFIABLE"
    models = sorted(sorted(wit["Value"]) for wit in out["Call"][0]["Witnesses"])
    assert models == [["__csp(x,2)"], ["__csp(x,2)", "p"]]


def test_unsat():
    """
    Test solving an unsatisfiable program.
    """
    res = run_app("--outf=2", program="&dom { 1..2 } = x. &sum { x } >= 3.")
    assert res.returncode == 20
    assert json.loads(res.stdout)["Result"] == "UNSATISFIABLE"


def test_error():
    """
    Test passing an unknown option.
    """
    res = run_app("--unknown-option")
    assert res.returncode == 65
    assert "unknown option" in res.stderr
