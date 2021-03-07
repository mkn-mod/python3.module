/**
Copyright (c) 2013, Philip Deegan.
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
#include <unordered_set>
#include "maiken/module/init.hpp"

namespace mkn {
namespace lang {

class PyBind11Module : public maiken::Module {
 private:
#if defined(_WIN32)
  const bool config_expected = 0;
#else
  const bool config_expected = 1;
#endif
  bool pyconfig_found = 0;
  std::string HOME, PY = "python3", PYTHON, PY_CONFIG = "python-config",
                    PY3_CONFIG = "python3-config", PATH = kul::env::GET("PATH");
  kul::Dir bin;
  std::shared_ptr<kul::cli::EnvVar> path_var;

 protected:
  static void VALIDATE_NODE(const YAML::Node& node) {
    using namespace kul::yaml;
    Validator({NodeValidator("args")}).validate(node);
  }

 public:
  void init(maiken::Application& a, const YAML::Node& node)
      KTHROW(std::exception) override {
    bool finally = 0;
    if (!kul::env::WHICH(PY.c_str())) PY = "python";
    PYTHON = kul::env::GET("PYTHON");
    if (!PYTHON.empty()) PY = PYTHON;
#if defined(_WIN32)
    if (PY.rfind(".exe") == std::string::npos) PY += ".exe";
#endif
    kul::Process p(PY);
    kul::ProcessCapture pc(p);
    HOME = kul::env::GET("PYTHON3_HOME");
    if (!HOME.empty()) {
#if defined(_WIN32)
      bin = kul::Dir(HOME);
      if (!bin) KEXCEPT(kul::Exception, "$PYTHON3_HOME does not exist");
#else
      bin = kul::Dir("bin", HOME);
      if (!bin) KEXCEPT(kul::Exception, "$PYTHON3_HOME/bin does not exist");
#endif
      path_var = std::make_shared<kul::cli::EnvVar>("PATH", bin.real(),
                                                    kul::cli::EnvVarMode::PREP);
      kul::env::SET(path_var->name(), path_var->toString().c_str());
      p.var(path_var->name(), path_var->toString());
    };
#if defined(_WIN32)
    pyconfig_found = false;  // doesn't exist on windows (generally)
#else
    pyconfig_found = kul::env::WHICH(PY3_CONFIG.c_str());
#endif
    if (!pyconfig_found) {
      pyconfig_found = kul::env::WHICH(PY_CONFIG.c_str());
      PY3_CONFIG = PY_CONFIG;
    }
    try {
      p << "-c"
        << "\"import sys; print(sys.version_info[0])\"";
      p.start();

      if (!pyconfig_found && config_expected) {
        finally = 1;
        KEXCEPT(kul::Exception, "python-config does not exist on path");
      }
    } catch (const kul::Exception& e) {
      KERR << e.stack();
    } catch (const std::exception& e) {
      KERR << e.what();
    } catch (...) {
      KERR << "UNKNOWN ERROR CAUGHT";
    }
    if (finally) exit(2);
    using namespace kul::cli;
    auto& evs = a.envVars();
    std::string extension;
    if (pyconfig_found) {
      kul::os::PushDir pushd(a.project().dir());
      kul::Process p(PY3_CONFIG);
      kul::ProcessCapture pc(p);
      p << "--extension-suffix";
      if (path_var) p.var(path_var->name(), path_var->toString());
      p.start();
      extension = pc.outs();
    } else {
      kul::Process p(PY);
      kul::ProcessCapture pc(p);
      p << "-c"
        << "\"import sysconfig; "
           "print(sysconfig.get_config_var('EXT_SUFFIX'))\"";
      p.start();
      extension = pc.outs();
    }
    a.m_cInfo.lib_ext = kul::String::LINES(extension)[0];  // drop EOL
    a.m_cInfo.lib_prefix = "";
    a.mode(maiken::compiler::Mode::SHAR);
  }
};
}  // namespace lang
}  // namespace mkn

extern "C" KUL_PUBLISH maiken::Module* maiken_module_construct() {
  return new mkn::lang::PyBind11Module;
}

extern "C" KUL_PUBLISH void maiken_module_destruct(maiken::Module* p) {
  delete p;
}
