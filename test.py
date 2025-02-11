#!/usr/bin/env python3

# Copyright 2021 Alexey Kutepov <reximkut@gmail.com> and Porth Contributors
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import argparse
import sys
import os
from os import path
import subprocess
from typing import List, BinaryIO, Optional
from dataclasses import dataclass, field

COMPILER_PATH = "./build/Neon"
NEON_EXT = ".ne"

OK_COLOR = "\033[92m"
WARNING_COLOR = "\033[93m"
ERROR_COLOR = "\033[91m"
RESET_COLOR = "\033[0m"

INFO = f"{OK_COLOR}INFO{RESET_COLOR}"
WARNING = f"{WARNING_COLOR}WARNING{RESET_COLOR}"
PASS = f"{OK_COLOR}PASS{RESET_COLOR}"
ERROR = f"{ERROR_COLOR}ERROR{RESET_COLOR}"
FAILURE = f"{ERROR_COLOR}FAILURE{RESET_COLOR}"
DOESNT_BUILD = f"{ERROR_COLOR}FAILURE (doesn\'t build){RESET_COLOR}"
COMPILER_CRASH = f"{ERROR_COLOR}FAILURE (compiler crashed){RESET_COLOR}"
DIDNT_CRASH = f"{OK_COLOR}PASS (didn't crash){RESET_COLOR}"

NON_OPTIMIZED = "non-optimized"
OPTIMIZED = "optimized"
DEBUG_SYMBOLS = "with debug symbols"

target = "./tests/"
output_target = "./tests_build/"

def cmd_run(cmd, **kwargs):
    return subprocess.run(cmd, **kwargs)

def read_blob_field(f: BinaryIO, name: bytes) -> bytes:
    line = f.readline()
    field = b':b ' + name + b' '
    assert line.startswith(field)
    assert line.endswith(b'\n')
    size = int(line[len(field):-1])
    blob = f.read(size)
    assert f.read(1) == b'\n'
    return blob

def read_int_field(f: BinaryIO, name: bytes) -> int:
    line = f.readline()
    field = b':i ' + name + b' '
    assert line.startswith(field)
    assert line.endswith(b'\n')
    return int(line[len(field):-1])

def write_int_field(f: BinaryIO, name: bytes, value: int):
    f.write(b':i %s %d\n' % (name, value))

def write_blob_field(f: BinaryIO, name: bytes, blob: bytes):
    f.write(b':b %s %d\n' % (name, len(blob)))
    f.write(blob)
    f.write(b'\n')

@dataclass
class TestCase:
    builds: bool
    argv: List[str]
    stdin: bytes
    returncode: int
    stdout: bytes
    stderr: bytes

DEFAULT_TEST_CASE = TestCase(builds=True, argv=[], stdin=bytes(), returncode=0, stdout=bytes(), stderr=bytes())

def load_test_case(file_path: str) -> Optional[TestCase]:
    try:
        with open(file_path, "rb") as f:
            argv = []
            builds = bool(read_int_field(f, b"builds"))
            argc = read_int_field(f, b"argc")
            for index in range(argc):
                argv.append(read_blob_field(f, b"arg%d" % index).decode("utf-8"))
            stdin = read_blob_field(f, b"stdin")
            returncode = read_int_field(f, b"returncode")
            stdout = read_blob_field(f, b"stdout")
            stderr = read_blob_field(f, b"stderr")
            return TestCase(builds, argv, stdin, returncode, stdout, stderr)
    except FileNotFoundError:
        return None

def save_test_case(file_path: str, builds: bool, argv: List[str], stdin: bytes, returncode: int, stdout: bytes, stderr: bytes):
    with open(file_path, "wb") as f:
        write_int_field(f, b"builds", int(builds))
        write_int_field(f, b"argc", len(argv))
        for index, arg in enumerate(argv):
            write_blob_field(f, b'arg%d' % index, arg.encode("utf-8"))
        write_blob_field(f, b"stdin", stdin)
        write_int_field(f, b"returncode", returncode)
        write_blob_field(f, b"stdout", stdout)
        write_blob_field(f, b"stderr", stderr)

@dataclass
class RunStats:
    passed: int = 0
    failed: int = 0
    ignored: int = 0
    ignored_files: List[str] = field(default_factory=list)
    failed_files: List[str] = field(default_factory=list)

def run_pass(file_path: str, tc: TestCase, stats: RunStats, compiler_args, pass_type: str):
    human_test_name = f"`{file_path[len(target):-len(NEON_EXT)]}`"

    print(f"{INFO}: Testing {human_test_name} ({pass_type}): ", end="")

    output_location = os.path.join(output_target, os.path.dirname(file_path)[len(target):])
    os.makedirs(output_location, exist_ok=True)

    output_filename = os.path.join(output_location, os.path.basename(file_path)[:-len(NEON_EXT)])

    compilation = cmd_run([COMPILER_PATH, *compiler_args, "-o", output_filename, file_path], capture_output=True)
    if compilation.returncode != 0:
        stats.failed += 1
        if compilation.returncode == -11:
            print(COMPILER_CRASH)
            stats.failed_files.append(f"{human_test_name} ({pass_type}, compiler crash, segfault)")
            return
        elif compilation.returncode == -6:
            print(COMPILER_CRASH)
            stats.failed_files.append(f"{human_test_name} ({pass_type}, compiler crash, aborted)")
            return

        stats.failed_files.append(f"{human_test_name} ({pass_type})")
        print(DOESNT_BUILD)
        return

    application = cmd_run([output_filename, *tc.argv], input=tc.stdin, capture_output=True)

    if application.returncode != tc.returncode:
        print(FAILURE)
        print(f"{ERROR}: Unexpected return code:")
        print(f"  Expected: {tc.returncode}")
        print(f"  Actual: {application.returncode}")
        stats.failed_files.append(f"{human_test_name} ({pass_type}, return code)")
        stats.failed += 1
        return

    if application.stdout != tc.stdout:
        print(FAILURE)
        print(f"{ERROR}: Unexpected stdout:")
        print("  Expected: {x!r}".format(x=tc.stdout))
        print(f"  Actual: {application.stdout}")
        stats.failed_files.append(f"{human_test_name} ({pass_type}, stdout)")
        stats.failed += 1
        return

    if application.stderr != tc.stderr:
        print(FAILURE)
        print(f"{ERROR}: Unexpected stderr:")
        print("  Expected: {x!r}".format(x=tc.stderr))
        print(f"  Actual: {application.stderr}")
        stats.failed_files.append(f"{human_test_name} ({pass_type}, stderr)")
        stats.failed += 1
        return

    stats.passed += 1
    print(PASS)

def run_test_for_file(file_path: str, stats: RunStats = RunStats()):
    assert path.isfile(file_path)
    assert file_path.endswith(NEON_EXT)

    human_test_name = f"`{file_path[len(target):-len(NEON_EXT)]}`"

    tc_path = file_path[:-len(NEON_EXT)] + ".txt"
    tc = load_test_case(tc_path)

    # NOTE: Even though we expect this to fail, it might still produce an output file.
    output_location = os.path.join(output_target, os.path.dirname(file_path)[len(target):])
    os.makedirs(output_location, exist_ok=True)

    output_filename = os.path.join(output_location, os.path.basename(file_path)[:-len(NEON_EXT)])

    if tc is None:
        print(f"{WARNING}: Could not find test case data for {human_test_name}. Only making sure the compiler doesn't crash: ", end="")
        compilation = cmd_run([COMPILER_PATH, "-o", output_filename, file_path], capture_output=True)
        if compilation.returncode in [0, 1]:
            print(DIDNT_CRASH)
        else:
            print(COMPILER_CRASH)

        stats.ignored_files.append(human_test_name)
        stats.ignored += 1
        return

    if not tc.builds:
        print(f"{INFO}: Testing {human_test_name} expected build fail: ", end="")

        compilation = cmd_run([COMPILER_PATH, "-o", output_filename, file_path], capture_output=True)
        if compilation.returncode == 1:
            stats.passed += 1
            print(PASS)
        elif compilation.returncode == 0:
            stats.failed_files.append(f"{human_test_name} (expected build fail)")
            stats.failed += 1
            print(FAILURE)
        else:
            stats.failed_files.append(f"{human_test_name} (compiler crash)")
            stats.failed += 1
            print(COMPILER_CRASH)
        return

    run_pass(file_path, tc, stats, [], NON_OPTIMIZED)
    run_pass(file_path, tc, stats, ["-O"], OPTIMIZED)
    run_pass(file_path, tc, stats, ["-g"], DEBUG_SYMBOLS)
    # TODO: Test validity of the IR with llc

def run_test_for_subfolder(folder: str, stats: RunStats):
    for entry in os.scandir(folder):
        if entry.is_file() and entry.path.endswith(NEON_EXT):
            run_test_for_file(entry.path, stats)
        elif entry.is_dir():
            run_test_for_subfolder(entry.path, stats)

def run_test_for_folder(folder: str):
    stats = RunStats()

    for entry in os.scandir(folder):
        if entry.is_file() and entry.path.endswith(NEON_EXT):
            run_test_for_file(entry.path, stats)
        elif entry.is_dir():
            run_test_for_subfolder(entry.path, stats)

    print()
    print(f"Passed: {OK_COLOR}{stats.passed}{RESET_COLOR}")
    print(f"Ignored: {WARNING_COLOR}{stats.ignored}{RESET_COLOR}")
    print(f"Failed: {ERROR_COLOR}{stats.failed}{RESET_COLOR}")
    print()

    if stats.ignored != 0:
        print("Ignored files:")
        for ignored_file in stats.ignored_files:
            print(f"    {ignored_file}")
        if stats.failed != 0:
            print()

    if stats.failed != 0:
        print("Failed files:")
        for failed_file in stats.failed_files:
            print(f"    {failed_file}")
        exit(1)

def update_input_for_file(file_path: str, argv: List[str]):
    assert file_path.endswith(NEON_EXT)
    tc_path = file_path[:-len(NEON_EXT)] + ".txt"
    tc = load_test_case(tc_path) or DEFAULT_TEST_CASE

    print(f"{INFO} Provide the stdin for the test case. Press ^D when you are done...")

    stdin = sys.stdin.buffer.read()

    print(f"{INFO} Saving input to {tc_path}")
    save_test_case(tc_path, tc.builds, argv, stdin, tc.returncode, tc.stdout, tc.stderr)

def update_output_for_file(file_path: str):
    tc_path = file_path[:-len(NEON_EXT)] + ".txt"
    tc = load_test_case(tc_path) or DEFAULT_TEST_CASE

    human_test_name = f"`{file_path[len(target):-len(NEON_EXT)]}`"

    output_location = os.path.join(output_target, os.path.dirname(file_path)[len(target):])
    os.makedirs(output_location, exist_ok=True)

    output_filename = os.path.join(output_location, os.path.basename(file_path)[:-len(NEON_EXT)])

    compilation = cmd_run([COMPILER_PATH, "-o", output_filename, file_path], capture_output=True)

    if compilation.returncode == 0:
        output = cmd_run([output_filename, *tc.argv], input=tc.stdin, capture_output=True)
        print(f"{INFO} Saving output for {human_test_name} to {tc_path}")
        save_test_case(tc_path, True, tc.argv, tc.stdin, output.returncode, output.stdout, output.stderr)
    elif compilation.returncode == -11:
        print(f"{WARNING}: Compiler crashed on {human_test_name} (segfault). Not saving the output.")
    elif compilation.returncode == -6:
        print(f"{WARNING}: Compiler crashed on {human_test_name} (aborted). Not saving the output.")
    else:
        print(f"{INFO} Saving output for {human_test_name} to {tc_path}")
        save_test_case(tc_path, False, tc.argv, tc.stdin, tc.returncode, tc.stdout, tc.stderr)


def update_output_for_folder(folder: str):
    for entry in os.scandir(folder):
        if entry.is_file() and entry.path.endswith(NEON_EXT):
            update_output_for_file(entry.path)
        elif entry.is_dir():
            update_output_for_folder(entry.path)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run or update the tests.')
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-u", "--update", action="store_true", help="update the output of the tests")
    group.add_argument("--update-input", action="store_true", help="update the input of the tests")
    parser.add_argument("-o", "--output", help="output target", default="./tests_build/")
    parser.add_argument("target", help="target to run the tests on", default="./tests/", nargs='?')
    args = parser.parse_args()

    target = args.target
    output_target = args.output

    if target == output_target:
        print(f"{ERROR}: target and output cannot be the same")
        exit(1)

    if not path.exists(target):
        print(f"{ERROR}: {target} does not exist")
        exit(1)

    if not path.exists(output_target):
        print(f"{INFO}: Creating output target {output_target}")
        os.makedirs(output_target)

    if args.update:
        if path.isdir(target):
            update_output_for_folder(target)
        elif path.isfile(target):
            update_output_for_file(target)
        else:
            assert False, 'unreachable'
        exit(0)

    if args.update_input:
        if path.isfile(target):
            update_input_for_file(target, [])
        else:
            assert False, 'unreachable'
        exit(0)

    if path.isdir(target):
        run_test_for_folder(target)
    elif path.isfile(target):
        run_test_for_file(target)
    else:
        assert False, 'unreachable'
