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

#include <string>    // for basic_string, string
#include <vector>    // for vector
#include <stdlib.h>  // for exit
#include <stdexcept>

namespace YAML {
class Node;
}

namespace mkn {
namespace python3 {

kul::File find_python3() KTHROW(std::exception) {
  std::string const HOME = kul::env::GET("PYTHON3_HOME");

  kul::Dir dir;
  if (!HOME.empty()) {
#if defined(_WIN32)
    dir = kul::Dir(HOME);
    if (!dir) KEXCEPT(kul::Exception, "$PYTHON3_HOME does not exist");
#else
    dir = kul::Dir("bin", HOME);
    if (!dir) KEXCEPT(kul::Exception, "$PYTHON3_HOME/bin does not exist");
#endif
  }

  std::vector<std::string> bins{"python3", "python"};
  for (auto const& bin : bins)
    if (kul::env::WHICH(bin.c_str()))
      return dir ? kul::File{bin, dir} : kul::env::WHERE(bin.c_str());

#if defined(_WIN32)  // or fallback
  std::vector<std::string> exes{"python3.exe", "python.exe"};
  for (auto const& bin : exes)
    if (kul::env::WHICH(bin.c_str()))
      return dir ? kul::File{bin, dir} : kul::env::WHERE(bin.c_str());
#endif

  throw std::runtime_error("Could not find python!");
}

kul::cli::EnvVar python3_path_var(kul::File const& exe) {
  return {"PATH", exe.dir().real(), kul::cli::EnvVarMode::PREP};
}

static inline kul::File const python_exe = find_python3();

std::string pyexec_for_string(std::string const& cmd) {
  auto const path_var = python3_path_var(python_exe);
  kul::Process p(python_exe.real());
  kul::ProcessCapture pc(p);
  p << "-c" << ("\"" + cmd + "\"");
  p.var(path_var.name(), path_var.toString());

  try {
    p.start();
  } catch (kul::proc::ExitException const& ex) {
    KLOG(ERR) << pc.outs();
    KLOG(ERR) << pc.errs();
    KERR << ex;
    throw;
  }
  auto ret = kul::String::LINES(pc.outs())[0];
  if(ret.back() == '\n') ret.pop_back();
  if(ret.back() == '\r') ret.pop_back();
  return ret;
}

class ModuleMaker : public maiken::Module {
 public:
  void init(maiken::Application& a, YAML::Node const& /*node*/) KTHROW(std::exception) override;
};

void ModuleMaker::init(maiken::Application& a, YAML::Node const& /*node*/) KTHROW(std::exception) {
  auto const cmd = "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))";
  a.m_cInfo.lib_ext = pyexec_for_string(cmd);
  a.m_cInfo.lib_prefix = "";
  a.mode(maiken::compiler::Mode::SHAR);
}

}  // namespace python3
}  // namespace mkn

extern "C" MKN_KUL_PUBLISH maiken::Module* maiken_module_construct() {
  return new mkn::python3::ModuleMaker;
}

extern "C" MKN_KUL_PUBLISH void maiken_module_destruct(maiken::Module* p) { delete p; }
