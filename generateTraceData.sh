#!/bin/bash

llvm-xray-11 convert -f trace_event xray-log.main.* -symbolize --instr_map main > trace.json
rm xray-log.main.*

