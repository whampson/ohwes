#!/bin/bash

# Makefile Autocompletion
# https://stackoverflow.com/a/38415982
complete -W "\`grep -oE '^[a-zA-Z0-9_.-]+:([^=]|$)' ?akefile | sed 's/[^a-zA-Z0-9_.-]*$//'\`" make
