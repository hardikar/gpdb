#!/bin/bash -l

set -eox pipefail

CWDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "${CWDIR}/common.bash"

function prep_env() {
  source /opt/gcc_env.sh
  ln -s "$(pwd)/gpdb4_src/gpAux/ext/rhel5_x86_64/python-2.6.2" /opt
  export JAVA_HOME=/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.39.x86_64
  export PATH=${JAVA_HOME}/bin:${PATH}
}

function make_sync_tools() {
  pushd gpdb4_src/gpAux
    make sync_tools
    tar -czf ../../sync_tools_gpdb4/sync_tools_gpdb4.tar.gz ext
  popd
}

function build_gpdb() {
  pushd gpdb4_src/gpAux
    make GPROOT=/usr/local parallelexec_opts=-j"$(number_of_cores)" dist
  popd
}

function unittest_check_gpdb() {
  pushd gpdb4_src/gpAux
    make GPROOT=/usr/local unittest-check
  popd
}

function export_gpdb() {
  TARBALL=$(pwd)/bin_gpdb4/bin_gpdb4.tar.gz
  pushd /usr/local/greenplum-db-devel
  tar -czf ${TARBALL} *
  popd
}

function _main() {
  prep_env
  make_sync_tools
  build_gpdb
  unittest_check_gpdb
  export_gpdb
}

_main "$@"
