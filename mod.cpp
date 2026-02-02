/**
Copyright (c) 2024, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "maiken/module/init.hpp"  // IWYU pragma: keep

#include "maiken/app.hpp"       // for Application
#include "maiken/compiler.hpp"  // for CompilationInfo, Mode

#include "mkn/kul/os.hpp"      // for Dir, WHICH, PushDir
#include "mkn/kul/cli.hpp"     // for EnvVar, EnvVarMode
#include "mkn/kul/env.hpp"     // for GET, SET
#include "mkn/kul/defs.hpp"    // for MKN_KUL_PUBLISH
#include "mkn/kul/proc.hpp"    // for Process, ProcessCapture, AProcess
#include "maiken/project.hpp"  // for Project
#include "mkn/kul/except.hpp"  // for Exception, KEXCEPT, KTHROW
#include "mkn/kul/string.hpp"  // for String

#include <memory>    // for shared_ptr, make_shared
#include <string>    // for basic_string, string
#include <vector>    // for vector
#include <stdlib.h>  // for exit

namespace YAML {
class Node;
}

namespace mkn {
namespace python3 {

class ModuleMaker : public maiken::Module {
 public:
  void init(maiken::Application& a, YAML::Node const& /*node*/) KTHROW(std::exception) override {
    if (!kul::env::WHICH(PY.c_str())) PY = "python";
    PYTHON = mkn::kul::env::GET("PYTHON");
    if (!PYTHON.empty()) PY = PYTHON;
#if defined(_WIN32)
    if (PY.rfind(".exe") == std::string::npos) PY += ".exe";
#endif
    mkn::kul::Process p(PY);
    mkn::kul::ProcessCapture pc(p);
    HOME = mkn::kul::env::GET("PYTHON3_HOME");
    if (!HOME.empty()) {
#if defined(_WIN32)
      bin = mkn::kul::Dir(HOME);
      if (!bin) KEXCEPT(kul::Exception, "$PYTHON3_HOME does not exist");
#else
      bin = mkn::kul::Dir("bin", HOME);
      if (!bin) KEXCEPT(kul::Exception, "$PYTHON3_HOME/bin does not exist");
#endif
      path_var =
          std::make_shared<kul::cli::EnvVar>("PATH", bin.real(), mkn::kul::cli::EnvVarMode::PREP);
      mkn::kul::env::SET(path_var->name(), path_var->toString().c_str());
      p.var(path_var->name(), path_var->toString());
    };

    auto const extension =
        pyexec_for_string("\"import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))\"");

    a.m_cInfo.lib_ext = mkn::kul::String::LINES(extension)[0];  // drop EOL
    a.m_cInfo.lib_prefix = "";
    a.mode(maiken::compiler::Mode::SHAR);
  }

 private:
  std::string pyexec_for_string(std::string const& cmd) {
    mkn::kul::Process p(PY);
    mkn::kul::ProcessCapture pc(p);
    p << "-c" << cmd;
    p.start();
    return pc.outs();
  }

  std::string HOME, PY = "python3", PYTHON, PY_CONFIG = "python-config",
                    PY3_CONFIG = "python3-config", PATH = mkn::kul::env::GET("PATH");
  mkn::kul::Dir bin;
  std::shared_ptr<kul::cli::EnvVar> path_var;
};
}  // namespace python3
}  // namespace mkn

extern "C" MKN_KUL_PUBLISH maiken::Module* maiken_module_construct() {
  return new mkn::python3::ModuleMaker;
}

extern "C" MKN_KUL_PUBLISH void maiken_module_destruct(maiken::Module* p) { delete p; }
