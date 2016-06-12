import os
import subprocess
import sys

from GporcaCommon import GporcaCommon

LLVM_RPM_URL="https://github.com/hardikar/rpm-specs/releases/download/centos6/llvm37-3.7.1-4.el6.x86_64.rpm"
CLANG_RPM_URL="https://github.com/hardikar/rpm-specs/releases/download/centos6/clang-3.7.1-4.el6.x86_64.rpm"

class GporcaCodegen(GporcaCommon):
    def _download_and_install_rpm(self, url):
        status = subprocess.call(["curl", url, "-o", "t.rpm"])
        if status:
            return status
        status = subprocess.call(["rpm", "-i", "t.rpm"])
        if status:
            return status

    def install_system_deps(self):
        status = GporcaCommon.install_system_deps(self)
        if status:
            return status
        status = self._download_and_install_rpm(LLVM_RPM_URL)
        if status:
            return status
        status = self._download_and_install_rpm(CLANG_RPM_URL)
        if status:
            return status
        

    def configure(self):
        return subprocess.call(["./configure",
                                "--enable-orca",
                                "--enable-codegen",
                                "--enable-mapreduce",
                                "--with-perl",
                                "--with-libxml",
                                "--with-python",
                                "--disable-gpfdist",
                                "--prefix=/usr/local/gpdb"], cwd="gpdb_src")
    
