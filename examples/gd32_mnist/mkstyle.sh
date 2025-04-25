#!/bin/bash

ASTYLE_ARGS="--options=none --suffix=none --quiet \
             --style=linux --indent=force-tab=8 --pad-header --pad-oper --indent-preprocessor"

astyle ${ASTYLE_ARGS} src/* include/*

