#!/bin/env sh
llvm-xray-11 stack --all-stacks --stack-format flame --aggregation-type time --instr_map Executable xray-log.Executable.* | perl ../flamegraph.pl > graph.svg
