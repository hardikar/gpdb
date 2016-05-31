#!/bin/bash -l

set -eox pipefail

CWDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "${CWDIR}/common.bash"

function gen_env(){
  cat > /opt/run_test.sh <<-EOF
		trap look4results ERR

		function look4results() {

		    results_files="../gpMgmt/gpMgmt_testunit_results.log"

		    for results_file in \${results_files}; do
			if [ -f "\${results_file}" ]; then
			    cat <<-FEOF

						======================================================================
						RESULTS FILE: \${results_file}
						----------------------------------------------------------------------

						\$(cat "\${results_file}")

					FEOF
			fi
		    done
		    exit 1
		}
		source /usr/local/greenplum-db-devel/greenplum_path.sh
		source /opt/gcc_env.sh
		source \${1}/gpdb4_src/gpAux/gpdemo/gpdemo-env.sh
		cd \${1}/gpdb4_src/gpMgmt/bin
		make check
	EOF

	chmod a+x /opt/run_test.sh
}

function _main() {

    install_sync_tools
    configure
    install_gpdb
    ./gpdb4_src/ci/concourse/scripts/setup_gpadmin_user.bash
    make_cluster
    gen_env
    run_test
}

_main "$@"
