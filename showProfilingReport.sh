#!/bin/bash
set -e

make process_profile_data
llvm-cov-11 report ./main -instr-profile=default.profdata
llvm-cov-11 show ./main -instr-profile=default.profdata | less

