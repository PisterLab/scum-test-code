#!/bin/bash

SHELL := /bin/bash

.PHONY: \
	serial

serial:
	CWD = "$(pwd)"
	echo $(CWD)