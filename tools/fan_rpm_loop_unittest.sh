#!/bin/bash

function SourceModule() {
    # shellcheck source=tools/fan_rpm_loop_test.sh
    . fan_rpm_loop_test.sh
}

function SetupShims() {
    function MkDir() { echo "MkDir      $*"; }
    function Mv() { echo "Mv         $*"; }
    function Sleep() { echo "Sleep      $*"; }
    function SystemCtl() { echo "SystemCtl  $*"; }
    function CommandRpm() { echo "CommandRpm $*"; }
}

function TestRunRpmStepsWorks() {
    RunRpmSteps 1000 5000 3 30 || return
    RunRpmSteps 5000 1000 3 30 || return
    RunRpmSteps 1000 5000 1 30 || return
    RunRpmSteps 5000 1000 1 30 || return
}

function TestMainRejectsLowMinAndMax() {
    if main 0 0; then
        echo "main 0 0 not rejected?"
        return 1
    fi
    if main 1 0; then
        echo "main 1 0 not rejected?"
        return 1
    fi
}

function TestMainWorks() {
    main 1000 5005 || return
}

function main() {
    SourceModule                || return
    SetupShims                  || return
    TestRunRpmStepsWorks        || return
    TestMainRejectsLowMinAndMax || return
    TestMainWorks               || return
    echo "All tests completed."
}

if [ "$0" = "${BASH_SOURCE[0]}" ]; then
    # not sourced, execute main function
    main "$@"
fi

